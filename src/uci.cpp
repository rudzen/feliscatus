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

#include <iostream>
#include <string>

#include "uci.h"
#include "util.h"
#include "game.h"
#include "position.h"
#include "search.h"

UCIProtocol::UCIProtocol(ProtocolListener *cb, Game *g) : Protocol(cb, g) {}

void UCIProtocol::post_moves(const Move bestmove, const Move pondermove) {
  fmt::memory_buffer buffer;

  fmt::format_to(buffer, "bestmove {}", display_uci(bestmove));

  if (pondermove)
    fmt::format_to(buffer, " ponder {}", display_uci(pondermove));

  fmt::print("{}\n", fmt::to_string(buffer));
}

void UCIProtocol::post_info(const int d, const int selective_depth, const uint64_t node_count, const uint64_t nodes_per_sec, const TimeUnit time, const int hash_full) {
  fmt::print("info depth {} seldepth {} hashfull {} nodes {} nps {} time {}\n", d, selective_depth, hash_full, node_count, nodes_per_sec, time);
}

void UCIProtocol::post_curr_move(const Move curr_move, const int curr_move_number) {
  fmt::print("info currmove {} currmovenumber {}\n", display_uci(curr_move), curr_move_number);
}

void UCIProtocol::post_pv(const int d, const int max_ply, const uint64_t node_count, const uint64_t nodes_per_second, const TimeUnit time, const int hash_full, const int score, const std::array<PVEntry, MAXDEPTH> &pv, const int pv_length, const int ply, const NodeType node_type) {

  fmt::memory_buffer buffer;
  fmt::format_to(buffer, "info depth {} seldepth {} score cp {} ", d, max_ply, score);

  if (node_type == ALPHA)
    fmt::format_to(buffer, "upperbound ");
  else if (node_type == BETA)
    fmt::format_to(buffer, "lowerbound ");

  fmt::format_to(buffer, "hashfull {} nodes {} nps {} time {} pv ", hash_full, node_count, nodes_per_second, time);

  for (auto i = ply; i < pv_length; ++i)
    fmt::format_to(buffer, "{} ", pv[i].move);

  fmt::print("{}\n", fmt::to_string(buffer));
}

int UCIProtocol::handle_go(std::istringstream &input) {

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

void UCIProtocol::handle_position(Game *g, std::istringstream &input) const {

  std::string token;

  input >> token;

  if (token == "startpos")
  {
    g->set_fen(Game::kStartPosition);

    // get rid of "moves" token
    input >> token;
  } else if (token == "fen")
  {
    std::string fen;
    while (input >> token && token != "moves")
      fen += token + ' ';
    g->set_fen(fen);
  } else
    return;

  auto m{MOVE_NONE};

  // parse any moves if they exist
  while (input >> token && (m = *g->pos->string_to_move(token)) != MOVE_NONE)
    g->make_move(m, false, true);
}

bool UCIProtocol::handle_set_option(std::istringstream &input) const {

  std::string token, option_name, option_value;

  // get rid of "name"
  input >> token;

  // read possibly spaced name
  while (input >> token && token != "value")
    option_name += (option_name.empty() ? "" : " ") + token;

  // read possibly spaced value
  while (input >> token)
    option_value += (option_value.empty() ? "" : " ") + token;

  auto valid_option = !option_name.empty() || !option_value.empty();

  if (valid_option)
    valid_option = callback->set_option(option_name, option_value);

  return valid_option;
}
