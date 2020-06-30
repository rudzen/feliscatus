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

#pragma once

#include <array>
#include <iostream>
#include <string>

#include <fmt/format.h>

#include "miscellaneous.h"
#include "types.h"
#include "pv_entry.h"
#include "search_limits.h"

struct Felis;
struct Game;

namespace uci {

  void post_moves(Move bestmove, Move pondermove);

  void post_info(int d, int selective_depth, TimeUnit time, int hash_full);

  void post_curr_move(Move curr_move, int curr_move_number);

  void post_pv(int d, int max_ply, int score, const std::array<PVEntry, MAXDEPTH> &pv, int pv_length, int ply, NodeType node_type);

  int handle_go(std::istringstream& input, SearchLimits &limits);

  void handle_position(Game *g, std::istringstream& input);

  bool handle_set_option(std::istringstream& input, Felis *felis);

  std::string display_uci(const Move m);

}

///
/// Move formatter
///
template<>
struct fmt::formatter<Move> : formatter<std::string_view> {
  // parse is inherited from formatter<string_view>.
  template<typename FormatContext>
  auto format(const Move c, FormatContext &ctx) {
    return formatter<std::string_view>::format(uci::display_uci(c), ctx);
  }
};

///
/// Square formatter
///
template<>
struct fmt::formatter<Square> : formatter<std::string_view> {
  // parse is inherited from formatter<string_view>.
  template<typename FormatContext>
  auto format(const Square sq, FormatContext &ctx) {
    return formatter<std::string_view>::format(SquareString[sq], ctx);
  }
};

///
/// File formatter
///
template<>
struct fmt::formatter<File> : formatter<std::string_view> {
  // parse is inherited from formatter<string_view>.
  template<typename FormatContext>
  auto format(const File f, FormatContext &ctx) {
    return formatter<std::string_view>::format('a' + static_cast<char>(f), ctx);
  }
};
