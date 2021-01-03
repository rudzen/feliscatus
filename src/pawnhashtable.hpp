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

#include "types.hpp"
#include "hash.hpp"
#include "score.hpp"
#include "miscellaneous.hpp"

struct Board;

#pragma pack(1)
struct alignas(CacheLineSize / 2) PawnHashEntry final
{
  Key zkey{};
  std::array<Score, COL_NB> scores{};
  std::array<Bitboard, COL_NB> passed_pawns{};

  // TODO : Move more pawn-related only things here
  [[nodiscard]]
  Score eval() const noexcept
  {
    return scores[WHITE] - scores[BLACK];
  }

};
#pragma pack()

using PawnHashTable = Table<PawnHashEntry, 131072>;

namespace Pawn
{
template<bool Tuning>
[[nodiscard]]
PawnHashEntry *at(Board *b);

}   // namespace Pawn
