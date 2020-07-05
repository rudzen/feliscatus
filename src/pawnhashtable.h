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

#include <cstdint>

#include "types.h"
#include "hash.h"
#include "score.h"

struct Board;

#pragma pack(1)
struct PawnHashEntry final {
  Key zkey;
  Score eval;
  std::array<Bitboard, COL_NB> passed_pawns;
  [[no_unique_address]] int unused;
};
#pragma pack()

using PawnHashTable = Table<PawnHashEntry, 131072>;

namespace Pawn {
  [[nodiscard]]
  PawnHashEntry *find(Board *s);

  [[nodiscard]]
  PawnHashEntry *insert(Board *b, Score s, const std::array<Bitboard, 2> &passed_pawns);
}

// struct PawnHashTable final : Table<PawnHashEntry, 8192> {
// };
