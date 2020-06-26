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

#include <string_view>

#include "moves.h"
#include "material.h"

class HashEntry;

struct Position : Moves {
  void clear();

  [[nodiscard]]
  const Move *string_to_move(std::string_view m);

  [[nodiscard]]
  bool is_draw() const;

  [[nodiscard]]
  bool can_castle() const;

  [[nodiscard]]
  bool can_castle(CastlingRight r) const;

  int reversible_half_move_count{};
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
};

inline bool Position::is_draw() const {
  return flags & Material::recognize_draw();
}

inline bool Position::can_castle() const {
  return castle_rights != 0;
}

inline bool Position::can_castle(const CastlingRight r) const {
  return castle_rights & r;
}