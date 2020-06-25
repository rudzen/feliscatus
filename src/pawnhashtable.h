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

struct Position;

#pragma pack(1)
struct PawnHashEntry final {

  Bitboard passed_pawn_file(const Color c) { return static_cast<Bitboard>(passed_pawn_files[c]); }

  Key zkey;
  int16_t eval_mg;
  int16_t eval_eg;
  std::array<uint8_t, COL_NB> passed_pawn_files;
  int16_t unused;
};
#pragma pack()

struct PawnHashTable final : Table<PawnHashEntry, 512 * sizeof(PawnHashEntry)> {
  [[nodiscard]]
  PawnHashEntry *find(const Position *pos);

  [[nodiscard]]
  PawnHashEntry *insert(Key key, int score_mg, int score_eg, const std::array<int, 2> &passed_pawn_files);
};
