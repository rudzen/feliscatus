/*
  Feliscatus, a UCI chess playing engine derived from Tomcat 1.0 (Bobcat 8.0)
  Copyright (C) 2008-2016 Gunnar Harms (Bobcat author)
  Copyright (C) 2017      FireFather (Tomcat author)
  Copyright (C) 2020-2022 Rudy Alex Kohn

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

#include <uci.hpp>
#include <board.hpp>
#include <transpositional.hpp>
#include <perft.hpp>
#include <moves.hpp>
#include <eval.hpp>
#include <polyglot.hpp>
#include <types.hpp>

namespace
{

constexpr TimeUnit time_safety_margin = 1;

[[nodiscard]]
std::unique_ptr<Board> new_board()
{
  const std::size_t num_threads = static_cast<std::size_t>(Options[uci::uciName<uci::UciOptions::THREADS>()]);
  pool.set(num_threads);
  auto board = std::make_unique<Board>();
  board->set_fen(start_position, pool.main());
  return board;
}

[[nodiscard]]
constexpr u64 nps(const u64 nodes, const TimeUnit time)
{
  return nodes * 1000 / time;
}

struct NodeInfo
{
  u64 nodes;
  u64 nps;
};

[[nodiscard]]
NodeInfo node_info(const TimeUnit time)
{
  const u64 nodes = pool.node_count();
  return {.nodes = nodes, .nps = nps(nodes, time)};
}

[[nodiscard]]
Move string_to_move(Board *b, const std::string_view m)
{
  Moves<> mg = Moves(b);
  mg.generate_moves();

  while (const MoveData *move_data = mg.next_move()) [[unlikely]]
    if (m == uci::displayUci(move_data->move))
      return move_data->move;
  return MOVE_NONE;
}

void position(Board *b, std::istringstream &input)
{
  std::string token;

  input >> token;

  [[likely]]
  if (token == "startpos")
  {
    b->new_game(pool.main());

    // get rid of "moves" token
    input >> token;
  } else if (token == "fen")
  {
    fmt::memory_buffer fen;
    auto inserter = std::back_inserter(fen);
    while (input >> token && token != "moves")
      fmt::format_to(inserter, "{} ", token);
    b->set_fen(fmt::to_string(fen), pool.main());
  } else
    return;

  // parse any moves if they exist
  while (input >> token) [[likely]]
    if (const auto m = string_to_move(b, token); m)
      b->make_move(m, false, true);
}

void set_option(std::istringstream &input)
{
  std::string token, option_name, option_value, output;

  // get rid of "name"
  input >> token;

  // read possibly spaced name
  while (input >> token && token != "value")
    option_name += (option_name.empty() ? "" : " ") + token;

  // read possibly spaced value
  while (input >> token)
    option_value += (option_value.empty() ? "" : " ") + token;

  [[likely]]
  if (Options.contains(option_name))
  {
    Options[option_name] = option_value;
    output               = fmt::format("Option {} = {}\n", option_name, option_value);
  } else
    output = fmt::format("Uknown option {} = {}\n", option_name, option_value);

  const auto uci_info = uci::info(output);
  fmt::print("{}", uci_info);
}

void go(std::istringstream &input, const std::string_view fen)
{
  SearchLimits *limits = pool.limits;

  ClearSearchLimits(limits);

  std::string token;

  while (input >> token)
    if (token == "wtime")
      input >> limits->time[WHITE];
    else if (token == "btime")
      input >> limits->time[BLACK];
    else if (token == "winc")
      input >> limits->inc[WHITE];
    else if (token == "binc")
      input >> limits->inc[BLACK];
    else if (token == "movestogo")
      input >> limits->movestogo;
    else if (token == "depth")
      input >> limits->depth;
    else if (token == "movetime")
      input >> limits->movetime;
    else if (token == "infinite")
      limits->infinite = true;
    else if (token == "ponder")
      limits->ponder = true;

  pool.start_thinking(fen);
}

}   // namespace

void uci::postMoves(const Move m, const Move ponderMove)
{
  fmt::memory_buffer buffer;
  auto inserter = std::back_inserter(buffer);

  fmt::format_to(inserter, "bestmove {}", displayUci(m));

  [[likely]]
  if (ponderMove)
    fmt::format_to(inserter, " ponder {}", displayUci(ponderMove));

  fmt::print("{}\n", fmt::to_string(buffer));
}

void uci::postInfo(const int d, const int selectiveDepth)
{
  const TimeUnit time     = elapsed(&pool.main()->time) + time_safety_margin;
  const NodeInfo nodeInfo = node_info(time);
  if (!Options[uciName<UciOptions::SHOW_CPU>()])
    fmt::print("info depth {} seldepth {} hashfull {} nodes {} nps {} time {}\n", d, selectiveDepth, TT.load(), nodeInfo.nodes, nodeInfo.nps, time);
  else
    fmt::print("info depth {} seldepth {} hashfull {} nodes {} nps {} time {} cpuload {}\n", d, selectiveDepth, TT.load(), nodeInfo.nodes, nodeInfo.nps, time, Cpu.usage());
}

void uci::postCurrMove(const Move m, int number)
{
  fmt::print("info currmove {} currmovenumber {}\n", displayUci(m), number);
}

void uci::postPv(int d, int maxPly, int score, const std::span<PVEntry> &pvLine, const NodeType nt)
{
  fmt::memory_buffer buffer;
  auto inserter = std::back_inserter(buffer);

  fmt::format_to(inserter, "info depth {} seldepth {} score cp {} ", d, maxPly, score);

  if (nt == ALPHA)
    fmt::format_to(inserter, "upperbound ");
  else if (nt == BETA)
    fmt::format_to(inserter, "lowerbound ");

  const TimeUnit time                       = elapsed(&pool.main()->time) + time_safety_margin;
  const auto [node_count, nodes_per_second] = node_info(time);

  fmt::format_to(inserter, "hashfull {} nodes {} nps {} time {} pv ", TT.load(), node_count, nodes_per_second, time);

  for (const PVEntry &pv : pvLine)
    fmt::format_to(inserter, "{} ", displayUci(pv.move));

  fmt::print("{}\n", fmt::to_string(buffer));
}

std::string uci::displayUci(const Move m)
{
  [[unlikely]]
  if (m == MOVE_NONE)
    return {"0000"};

  // append piece promotion if the move is a promotion.
  return !is_promotion(m) ? fmt::format("{}{}", square_to_string(move_from(m)), square_to_string(move_to(m))) : fmt::format("{}{}{}", square_to_string(move_from(m)), square_to_string(move_to(m)), piece_index[type_of(move_promoted(m))]);
}

std::string uci::info(const std::string_view infoString)
{
  return fmt::format("info string {}", infoString);
}

void uci::run(const int argc, char *argv[])
{
  std::setbuf(stdout, nullptr);

  auto board = new_board();
  std::string command;
  std::string token;

  // TODO : replace with CLI
  for (auto argument_index = 1; argument_index < argc; ++argument_index)
    command += fmt::format("{} ", argv[argument_index]);

  do
  {
    [[unlikely]]
    if (argc == 1 && !std::getline(std::cin, command))
      command = "quit";

    std::istringstream input(command);

    token.clear();
    input >> std::skipws >> token;

    [[unlikely]]
    if (token == "quit" || token == "stop")
      pool.stop = true;
    else if (token == "ponder")
      pool.main()->ponder = true;
    else if (token == "uci")
    {
      // auto output = fmt::format("{}{}\nuciok\n", misc::print_engine_info<true>(), Options);
      // fmt::print("{}{}\nuciok\n", misc::print_engine_info<true>(), Options);
      fmt::print("uciok\n");
    } else if (token == "isready")
      fmt::print("readyok\n");
    else if (token == "ucinewgame")
    {
      if (Options[uci::uciName<UciOptions::CLEAR_HASH_NEW_GAME>()])
        TT.clear();
      board = new_board();
      fmt::print("readyok\n");
    } else if (token == "setoption")
      set_option(input);
    else if (token == "position")
      position(board.get(), input);
    else if (token == "go")
      go(input, board->fen());
    else if (token == "perft")
    {
      const auto total = perft::perft(board.get(), 6);
      fmt::print("Total nodes: {}\n", total);
    } else if (token == "divide")
    {
      const auto total = perft::divide(board.get(), 6);
      fmt::print("Total nodes: {}\n", total);
    } else if (token == "print")
      board->print_moves();
    else if (token == "d")
      board->print();
    else if (token == "eval")
    {
      board->print();
      const auto e = Eval::evaluate(board.get(), 0, 0, 0);
      fmt::print("Eval: {}\n", e);
    } else if (token == "book")
    {
      const Move m = book.probe(board.get());
      postMoves(m, MOVE_NONE);
    } else if (token == "exit")
      break;
  } while (token != "quit" && argc == 1);
}
