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

#include "types.h"
#include "prng.h"

namespace zobrist {

inline Key zobrist_pst[14][64];
inline std::array<Key, 16> zobrist_castling{};
inline Key zobrist_side, zobrist_nopawn;
inline std::array<Key, 8> zobrist_ep_file{};

inline void init() {

  constexpr Key Seed = 1070372;

  PRNG<Key> rng(Seed);

  for (const auto p : PieceTypes)
  {
    for (const auto sq : Squares)
    {
      zobrist_pst[p][sq]     = rng();
      zobrist_pst[p + 8][sq] = rng();
    }
  }

  for (auto &i : zobrist_castling)
    i = rng();

  for (auto &i : zobrist_ep_file)
    i = rng();

  zobrist_side = rng();
  zobrist_nopawn = rng();
}
}// namespace zobrist