// Copyright (C) 2008-2016 Gunnar Harms (Bobcat author)
// Copyright (C) 2017      FireFather (Tomcat author)
// Copyright (C) 2020-2025 Rudy Alex Kohn
// See end of file for extended copyright information.

#pragma once

#include <felis/types.hpp>

struct Board;

struct Material {
  i32 material_value[COL_NB];
  i32 drawish;
  i32 material_flags;
  u32 key[COL_NB];
  const Board* board;
};

namespace material {

constexpr int RECOGNIZEDDRAW          = 1;
constexpr int max_value_without_pawns = 2 * (2 * piece_values[KNIGHT] + 2 * piece_values[BISHOP] + 2 * piece_values[ROOK] + piece_values[QUEEN]);
constexpr int max_value               = max_value_without_pawns + 2 * 8 * piece_values[PAWN];

void clear(Material* m);

void remove(Material* m, Piece pc);

void add(Material* m, Piece pc);

i32 count(const Material* m, Color c, PieceType pt);

void make_move(Material* m, Move move);

bool is_kx(const Material* m, Color c);

[[nodiscard]]
int value(const Material* m);

[[nodiscard]]
int pawn_value(const Material* m);

[[nodiscard]]
int pawn_count(const Material* m);

[[nodiscard]]
int evaluate(Material* m, int& flags, int eval, const Board* b, Color us);

}   // namespace material

// Feliscatus, a UCI chess playing engine derived from Tomcat 1.0 (Bobcat 8.0)
// Copyright (C) 2008-2016 Gunnar Harms (Bobcat author)
// Copyright (C) 2017      FireFather (Tomcat author)
// Copyright (C) 2020-2025 Rudy Alex Kohn
//
// Feliscatus is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Feliscatus is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Feliscatus.  If not, see <http://www.gnu.org/licenses/>.