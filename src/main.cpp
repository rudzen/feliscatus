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

#include "feliscatus.h"
#include "bitboard.h"
#include "magic.h"
#include "util.h"
#include "pawnhashtable.h"
#include "transpositional.h"
#include "uci.h"

constexpr std::size_t TT_ENTRY_SIZE   = 16;
constexpr std::size_t PAWN_ENTRY_SIZE = 16;

int main(const int argc, char *argv[]) {
  util::check_size<HashEntry, TT_ENTRY_SIZE>();
  util::check_size<PawnHashEntry, PAWN_ENTRY_SIZE>();

  bitboard::init();
  attacks::init();
  Board::init();
  uci::init(Options);

  TT.init(1);
  Felis engine;
  engine.run(argc, argv);
}