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

#include "position.hpp"
#include "bitboard.hpp"

void Position::clear()
{
  castle_rights              = 0;
  reversible_half_move_count = 0;
  pawn_structure_key = key = 0;
  last_move                = MOVE_NONE;
  null_moves_in_row        = 0;
  transposition            = nullptr;
  last_move                = MOVE_NONE;
  checkers                 = ZeroBB;
  in_check                 = false;
  previous                 = nullptr;
  material.clear();
  killer_moves.fill(MOVE_NONE);
}
