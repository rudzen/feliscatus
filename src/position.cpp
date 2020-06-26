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

#include "position.h"
#include "util.h"
#include "board.h"

namespace {

bool is_string_castle_move(const Board *b, const std::string_view m, int &castle_type) {
  if (m == "O-O" || m == "OO" || m == "0-0" || m == "00" || (m == "e1g1" && b->get_piece_type(e1) == King) || (m == "e8g8" && b->get_piece_type(e8) == King))
  {
    castle_type = 0;
    return true;
  }

  if (m == "O-O-O" || m == "OOO" || m == "0-0-0" || m == "000" || (m == "e1c1" && b->get_piece_type(e1) == King) || (m == "e8c8" && b->get_piece_type(e8) == King))
  {
    castle_type = 1;
    return true;
  }

  return false;
}

}// namespace

void Position::clear() {
  in_check                   = false;
  castle_rights              = 0;
  reversible_half_move_count = 0;
  pawn_structure_key = key   = 0;
  last_move                  = MOVE_NONE;
  null_moves_in_row          = 0;
  transposition              = nullptr;
  last_move                  = MOVE_NONE;
  material.clear();
}

const Move *Position::string_to_move(std::string_view m) {
  auto castle_type = -1;// 0 = short, 1 = long

  if (!is_string_castle_move(b, m, castle_type) && (!util::in_between<'a', 'h'>(m[0]) || !util::in_between<'1', '8'>(m[1]) || !util::in_between<'a', 'h'>(m[2]) || !util::in_between<'1', '8'>(m[3])))
    return nullptr;

  auto from = no_square;
  auto to   = no_square;

  if (castle_type == -1)
  {
    from = make_square(static_cast<File>(m[0] - 'a'), static_cast<Rank>(m[1] - '1'));
    to   = make_square(static_cast<File>(m[2] - 'a'), static_cast<Rank>(m[3] - '1'));

    // chess 960 - shredder fen
    if ((b->get_piece(from) == King && b->get_piece(to) == Rook) || (b->get_piece(from) == King + 8 && b->get_piece(to) == Rook + 8))
      castle_type = to > from ? 0 : 1;// ga na
  }

  if (castle_type == 0)
  {
    from = oo_king_from[side_to_move];
    to   = oo_king_to[side_to_move];
  } else if (castle_type == 1)
  {
    from = ooo_king_from[side_to_move];
    to   = ooo_king_to[side_to_move];
  }

  generate_moves();

  while (const MoveData *move_data = next_move())
  {
    const auto *const move = &move_data->move;

    if (move_from(*move) == from && move_to(*move) == to)
    {
      if (::is_castle_move(*move) && castle_type == -1)
        continue;

      if (is_promotion(*move))
        if (tolower(m.back()) != piece_notation[type_of(move_promoted(*move))].front())
          continue;

      return move;
    }
  }
  return nullptr;
}

bool Position::is_draw() const {
  return flags & Material::recognize_draw();
}
