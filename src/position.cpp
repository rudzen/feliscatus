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

#include <optional>
#include "position.h"
#include "util.h"
#include "board.h"

namespace {

std::optional<int> is_string_castle_move(const Board *b, const std::string_view m) {
  if (m == "O-O" || m == "OO" || m == "0-0" || m == "00" || (m == "e1g1" && b->get_piece_type(E1) == KING) || (m == "e8g8" && b->get_piece_type(E8) == KING))
    return std::make_optional(0);

  if (m == "O-O-O" || m == "OOO" || m == "0-0-0" || m == "000" || (m == "e1c1" && b->get_piece_type(E1) == KING) || (m == "e8c8" && b->get_piece_type(E8) == KING))
    return std::make_optional(1);

  return std::nullopt;
}

}// namespace

void Position::clear() {
  castle_rights              = 0;
  reversible_half_move_count = 0;
  pawn_structure_key = key   = 0;
  last_move                  = MOVE_NONE;
  null_moves_in_row          = 0;
  transposition              = nullptr;
  last_move                  = MOVE_NONE;
  checkers                   = ZeroBB;
  in_check                   = false;
  material.clear();
  killer_moves.fill(MOVE_NONE);
}

const Move *Position::string_to_move(const std::string_view m) {
  // 0 = short, 1 = long
  auto castle_type = is_string_castle_move(b, m);

  if (!castle_type && (!util::in_between<'a', 'h'>(m[0]) || !util::in_between<'1', '8'>(m[1]) || !util::in_between<'a', 'h'>(m[2]) || !util::in_between<'1', '8'>(m[3])))
    return nullptr;

  auto from = NO_SQ;
  auto to   = NO_SQ;

  if (!castle_type)
  {
    from = make_square(static_cast<File>(m[0] - 'a'), static_cast<Rank>(m[1] - '1'));
    to   = make_square(static_cast<File>(m[2] - 'a'), static_cast<Rank>(m[3] - '1'));

    // chess 960 - shredder fen
    if ((b->get_piece(from) == W_KING && b->get_piece(to) == W_ROOK) || (b->get_piece(from) == B_KING && b->get_piece(to) == B_ROOK))
      castle_type = to > from ? std::make_optional(0) : std::make_optional(1);// ga na
  }

  if (castle_type)
  {
    if (castle_type.value() == 0)
    {
      from = oo_king_from[side_to_move];
      to   = oo_king_to[side_to_move];
    } else if (castle_type.value() == 1)
    {
      from = ooo_king_from[side_to_move];
      to   = ooo_king_to[side_to_move];
    }
  }

  generate_moves();

  while (const MoveData *move_data = next_move())
  {
    const auto *const move = &move_data->move;

    if (move_from(*move) == from && move_to(*move) == to)
    {
      if (is_castle_move(*move) && !castle_type)
        continue;

      if (is_promotion(*move))
        if (tolower(m.back()) != piece_notation[type_of(move_promoted(*move))].front())
          continue;

      return move;
    }
  }
  return nullptr;
}
