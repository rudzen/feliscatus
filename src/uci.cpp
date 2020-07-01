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

#include "uci.h"
#include "util.h"
#include "board.h"
#include "position.h"
#include "search.h"
#include "feliscatus.h"
#include "datapool.h"
#include "transpositional.h"

namespace {

constexpr TimeUnit time_safety_margin = 1;

constexpr std::string_view fen_piece_names {"PNBRQK  pnbrqk "};

constexpr uint64_t nps(const uint64_t nodes, const TimeUnit time) {
  return nodes * 1000 / time;
}

auto node_info(const TimeUnit time) {
  const auto nodes = Pool.node_count();
  return std::make_pair(nodes, nps(nodes, time));
}

}

void uci::post_moves(const Move bestmove, const Move pondermove) {
  fmt::memory_buffer buffer;

  fmt::format_to(buffer, "bestmove {}", display_uci(bestmove));

  if (pondermove)
    fmt::format_to(buffer, " ponder {}", display_uci(pondermove));

  fmt::print("{}\n", fmt::to_string(buffer));
}

void uci::post_info(const int d, const int selective_depth) {
  const auto time = Pool.time.elapsed() + time_safety_margin;
  const auto [node_count, nodes_per_second] = node_info(time);
  fmt::print("info depth {} seldepth {} hashfull {} nodes {} nps {} time {}\n", d, selective_depth, TT.get_load(), node_count, nodes_per_second, time);
}

void uci::post_curr_move(const Move curr_move, const int curr_move_number) {
  fmt::print("info currmove {} currmovenumber {}\n", display_uci(curr_move), curr_move_number);
}

void uci::post_pv(const int d, const int max_ply, const int score, const std::array<PVEntry, MAXDEPTH> &pv, const int pv_length, const int ply, const NodeType node_type) {

  fmt::memory_buffer buffer;
  fmt::format_to(buffer, "info depth {} seldepth {} score cp {} ", d, max_ply, score);

  if (node_type == ALPHA)
    fmt::format_to(buffer, "upperbound ");
  else if (node_type == BETA)
    fmt::format_to(buffer, "lowerbound ");

  const auto time = Pool.time.elapsed() + time_safety_margin;
  const auto [node_count, nodes_per_second] = node_info(time);

  fmt::format_to(buffer, "hashfull {} nodes {} nps {} time {} pv ", TT.get_load(), node_count, nodes_per_second, time);

  for (auto i = ply; i < pv_length; ++i)
    fmt::format_to(buffer, "{} ", pv[i].move);

  fmt::print("{}\n", fmt::to_string(buffer));
}

int uci::handle_go(std::istringstream &input, SearchLimits &limits) {

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

  return 0;
}

void uci::handle_position(Board *b, std::istringstream &input) {

  std::string token;

  input >> token;

  if (token == "startpos")
  {
    b->set_fen(Board::kStartPosition);

    // get rid of "moves" token
    input >> token;
  } else if (token == "fen")
  {
    std::string fen;
    while (input >> token && token != "moves")
      fen += token + ' ';
    b->set_fen(fen);
  } else
    return;

  // parse any moves if they exist
  while (input >> token)
  {
    const auto *const m = b->pos->string_to_move(token);
      if (!m || *m == MOVE_NONE)
          break;
      b->make_move(*m, false, true);
  }
}

void uci::handle_set_option(std::istringstream &input) {

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
    output = "Option {} = {}\n";
  }
  else
    output = "Uknown option {} = {}\n";

  fmt::print(info(fmt::format(output, option_name, option_value)));
}

std::string uci::display_uci(const Move m) {

  if (m == MOVE_NONE)
    return "0000";

  // append piece promotion if the move is a promotion.
  return !is_promotion(m)
       ? fmt::format("{}{}", move_from(m), move_to(m))
       : fmt::format("{}{}{}", move_from(m), move_to(m), fen_piece_names[type_of(move_promoted(m))]);
}

std::string uci::info(const std::string_view info_string) {
  return fmt::format("info string {}", info_string);
}