/*
  Feliscatus, a UCI chess playing engine derived from Tomcat 1.0 (Bobcat 8.0)
  Copyright (C) 2008-2016 Gunnar Harms (Bobcat author)
  Copyright (C) 2017      FireFather (Tomcat author)
  Copyright (C) 2020      Rudy Alex Kohn

  Feliscatus is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Feliscatus is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <sstream>
#include <cstdio>

#include "uci.h"
#include "board.h"
#include "position.h"
#include "search.h"
#include "tpool.h"
#include "transpositional.h"
#include "miscellaneous.h"
#include "perft.h"

namespace {

constexpr TimeUnit time_safety_margin = 1;

std::unique_ptr<Board> new_board() {
  const auto num_threads = static_cast<std::size_t>(Options[uci::get_uci_name<uci::UciOptions::THREADS>()]);
  pool.set(num_threads);
  return std::make_unique<Board>(start_position, pool.main());
}

constexpr uint64_t nps(const uint64_t nodes, const TimeUnit time) {
  return nodes * 1000 / time;
}

auto node_info(const TimeUnit time) {
  const auto nodes = pool.node_count();
  return std::make_pair(nodes, nps(nodes, time));
}

Move string_to_move(Position *p, const std::string_view m) {
  p->generate_moves();

  while (const MoveData *move_data = p->next_move())
    if (m == uci::display_uci(move_data->move))
      return move_data->move;
  return MOVE_NONE;
}

void position(Board *b, std::istringstream &input) {

  std::string token;

  input >> token;

  [[likely]]
  if (token == "startpos")
  {
    b->new_game(pool.main());

    // get rid of "moves" token
    input >> token;
  }
  else if (token == "fen")
  {
    fmt::memory_buffer fen;
    while (input >> token && token != "moves")
      fmt::format_to(fen, "{} ", token);
    b->set_fen(fmt::to_string(fen), pool.main());
  }
  else return;

  // parse any moves if they exist
  while (input >> token)
    if (const auto m = string_to_move(b->pos, token); m)
      b->make_move(m, false, true);
}

void set_option(std::istringstream &input) {

  std::string token, option_name, option_value, output;

  // get rid of "name"
  input >> token;

  // read possibly spaced name
  while (input >> token && token != "value")
    option_name += (option_name.empty() ? "" : " ") + token;

  // read possibly spaced value
  while (input >> token)
    option_value += (option_value.empty() ? "" : " ") + token;

  if (Options.contains(option_name))
  {
    Options[option_name] = option_value;
    output               = "Option {} = {}\n";
  } else
    output = "Uknown option {} = {}\n";

  fmt::print(uci::info(fmt::format(output, option_name, option_value)));
}

void go(std::istringstream &input, std::string_view fen) {

  auto &limits = pool.limits;

  limits.clear();

  std::string token;

  while (input >> token)
    if (token == "wtime")
      input >> limits.time[WHITE];
    else if (token == "btime")
      input >> limits.time[BLACK];
    else if (token == "winc")
      input >> limits.inc[WHITE];
    else if (token == "binc")
      input >> limits.inc[BLACK];
    else if (token == "movestogo")
      input >> limits.movestogo;
    else if (token == "depth")
      input >> limits.depth;
    else if (token == "movetime")
      input >> limits.movetime;
    else if (token == "infinite")
      limits.infinite = true;
    else if (token == "ponder")
      limits.ponder = true;

  pool.start_thinking(fen);

}

}

void uci::post_moves(const Move m, const Move ponder_move) {
  fmt::memory_buffer buffer;

  fmt::format_to(buffer, "bestmove {}", display_uci(m));

  [[likely]]
  if (ponder_move)
    fmt::format_to(buffer, " ponder {}", display_uci(ponder_move));

  fmt::print("{}\n", fmt::to_string(buffer));
}

void uci::post_info(const int d, const int selective_depth) {
  const auto time = pool.main()->time.elapsed() + time_safety_margin;
  const auto [node_count, nodes_per_second] = node_info(time);
  if (!Options[get_uci_name<UciOptions::SHOW_CPU>()])
    fmt::print("info depth {} seldepth {} hashfull {} nodes {} nps {} time {}\n", d, selective_depth, TT.get_load(), node_count, nodes_per_second, time);
  else
    fmt::print("info depth {} seldepth {} hashfull {} nodes {} nps {} time {} cpuload {}\n", d, selective_depth, TT.get_load(), node_count, nodes_per_second, time, CpuLoad.usage());
}

void uci::post_curr_move(const Move m, const int m_number) {
  fmt::print("info currmove {} currmovenumber {}\n", display_uci(m), m_number);
}

void uci::post_pv(const int d, const int max_ply, const int score, const std::span<PVEntry> &pv_line, const NodeType nt) {

  fmt::memory_buffer buffer;
  fmt::format_to(buffer, "info depth {} seldepth {} score cp {} ", d, max_ply, score);

  if (nt == ALPHA)
    fmt::format_to(buffer, "upperbound ");
  else if (nt == BETA)
    fmt::format_to(buffer, "lowerbound ");

  const auto time = pool.main()->time.elapsed() + time_safety_margin;
  const auto [node_count, nodes_per_second] = node_info(time);

  fmt::format_to(buffer, "hashfull {} nodes {} nps {} time {} pv ", TT.get_load(), node_count, nodes_per_second, time);

  for (auto &pv : pv_line)
    fmt::format_to(buffer, "{} ", pv.move);

  fmt::print("{}\n", fmt::to_string(buffer));
}

std::string uci::display_uci(const Move m) {

  if (m == MOVE_NONE)
    return std::string("0000");

  // append piece promotion if the move is a promotion.
  return !is_promotion(m)
       ? fmt::format("{}{}", move_from(m), move_to(m))
       : fmt::format("{}{}{}", move_from(m), move_to(m), piece_index[type_of(move_promoted(m))]);
}

std::string uci::info(const std::string_view info_string) {
  return fmt::format("info string {}", info_string);
}

void uci::run(const int argc, char *argv[]) {
  std::setbuf(stdout, nullptr);

  auto board = new_board();
  std::string command;
  std::string token;

  // TODO : replace with CLI
  for (auto argument_index = 1; argument_index < argc; ++argument_index)
    command += fmt::format("{} ", argv[argument_index]);

  do
  {
    if (argc == 1 && !std::getline(std::cin, command))
      command = "quit";

    std::istringstream input(command);

    token.clear();
    input >> std::skipws >> token;

    if (token == "quit" || token == "stop")
      pool.stop = true;
    else if (token == "ponder")
      pool.main()->ponder = true;
    else if (token == "uci")
    {
      fmt::print("id name Feliscatus 0.1\n");
      fmt::print("id author Gunnar Harms, FireFather, Rudy Alex Kohn\n");
      fmt::print("{}\n", Options);
      fmt::print("uciok\n");
    } else if (token == "isready")
      fmt::print("readyok\n");
    else if (token == "ucinewgame")
    {
      if (Options[uci::get_uci_name<UciOptions::CLEAR_HASH_NEW_GAME>()])
        TT.clear();
      board = new_board();
      fmt::print("readyok\n");
    } else if (token == "setoption")
      set_option(input);
    else if (token == "position")
      position(board.get(), input);
    else if (token == "go")
    {
      //stop_threads();
      go(input, board->get_fen());
    } else if (token == "perft")
    {
      const auto total = perft::perft(board.get(), 6);
      fmt::print("Total nodes: {}\n", total);
    } else if (token == "quit" || token == "exit")
      break;
  } while (token != "quit" && argc == 1);

  // stop_threads();
}
