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

#include <fmt/format.h>

#include "pawnhashtable.hpp"
#include "board.hpp"
#include "parameters.hpp"

namespace Pawn
{

template<Color Us>
[[nodiscard]]
Score eval_pawns(const Board *b, PawnHashEntry *phe)
{
  constexpr auto Them   = ~Us;
  auto result           = ZeroScore;
  auto pawns            = b->pieces(PAWN, Us);
  phe->passed_pawns[Us] = 0;

  while (pawns)
  {
    const auto s      = pop_lsb(&pawns);
    const auto f      = file_of(s);
    const auto flip_s = relative_square(Them, s);

    result += pawn_pst[flip_s];

    if (b->is_pawn_passed(s, Us))
      phe->passed_pawns[Us] |= s;

    const auto open_file = !b->is_piece_on_file(PAWN, s, Them);

    if (b->is_pawn_isolated(s, Us))
      result += pawn_isolated[open_file];
    else if (b->is_pawn_behind(s, Us))
      result += pawn_behind[open_file];

    if (pawns & f)
      result += pawn_doubled[open_file];
  }
  return result;
}

template<>
PawnHashEntry *at<true>(const Board *b)
{
  const auto pawn_key = b->pawn_key();
  auto *entry = b->my_thread()->pawn_hash[pawn_key];

  entry->scores[WHITE] = eval_pawns<WHITE>(b, entry);
  entry->scores[BLACK] = eval_pawns<BLACK>(b, entry);
  entry->zkey = pawn_key;
  return entry;
}

template<>
PawnHashEntry *at<false>(const Board *b)
{
  const auto pawn_key = b->pawn_key();
  auto *entry = b->my_thread()->pawn_hash[pawn_key];

  if (entry->zkey == 0)
  {
    entry->scores[WHITE] = eval_pawns<WHITE>(b, entry);
    entry->scores[BLACK] = eval_pawns<BLACK>(b, entry);
    entry->zkey = pawn_key;
  }

  return entry;
}

}   // namespace Pawn
