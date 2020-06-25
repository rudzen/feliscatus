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

#include "types.h"
#include "util.h"

namespace squares {

inline void init() {

  for (const auto side : Colors)
  {
    const auto rank_1 = relative_rank(side, RANK_1);
    rook_castles_to[make_square(FILE_G, rank_1)] = make_square(FILE_F, rank_1);
    rook_castles_to[make_square(FILE_C, rank_1)] = make_square(FILE_D, rank_1);
  }

  // Arrays castle_right_mask, rook_castles_from, ooo_king_from and oo_king_from
  // are initd in method setupCastling of class Game.
}

}// namespace squares
