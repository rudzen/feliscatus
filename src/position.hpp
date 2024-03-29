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

#pragma once

#include "material.hpp"

struct HashEntry;

using KillerMoves = std::array<Move, 4>;

struct Position final
{
  void clear();

  int rule50{};
  Key pawn_structure_key{};
  Key key{};
  Material material{};
  int null_moves_in_row{};
  int pv_length{};
  Move last_move{};
  int eval_score{};
  int transp_score{};
  int transp_depth{};
  NodeType transp_type{};
  Move transp_move{};
  int flags{};
  HashEntry *transposition{};
  KillerMoves killer_moves{};
  Bitboard checkers{};
  bool in_check{};
  int castle_rights{};
  Square en_passant_square{};
  Color side_to_move{};
  Bitboard pinned{};
  Position *previous{};
};
