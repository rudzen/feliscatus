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

#include "pawnhashtable.h"
#include "position.h"

PawnHashEntry *PawnHashTable::find(const Position *pos) {
  return (*this)[pos->pawn_structure_key];
}

PawnHashEntry *PawnHashTable::insert(const Key key, const Score s, const std::array<int, 2> &passed_pawn_files) {
  auto *pawnp    = (*this)[key];
  pawnp->zkey    = key;
  pawnp->eval    = s;
  std::copy(passed_pawn_files.begin(), passed_pawn_files.end(), pawnp->passed_pawn_files.begin());
  return pawnp;
}
