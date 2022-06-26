/*
  Feliscatus, a UCI chess playing engine derived from Tomcat 1.0 (Bobcat 8.0)
  Copyright (C) 2008-2016 Gunnar Harms (Bobcat author)
  Copyright (C) 2017      FireFather (Tomcat author)
  Copyright (C) 2020-2022 Rudy Alex Kohn

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

#define CATCH_CONFIG_MAIN

#include <catch2/catch_all.hpp>

#include "../src/transpositional.hpp"
#include "../src/bitboard.hpp"
#include "../src/perft.hpp"
#include "../src/board.hpp"
#include "../src/miscellaneous.hpp"
#include "../src/tpool.hpp"

TEST_CASE("Perft basic", "[perft_basic]")
{
  TT.init(1);
  bitboard::init();
  Board::init();

  pool.set(1);

  Board b{};
  b.set_fen(start_position, pool.main());

  std::uint64_t result = 0;

  SECTION("Perft depth=1")
  {
    result = perft::perft(&b, 1);
    REQUIRE(result == 20);
  }

  SECTION("Perf depth=2")
  {
    result = perft::perft(&b, 2);
    REQUIRE(result == 20 + 400);
  }

  SECTION("Perf depth=3")
  {
    result = perft::perft(&b, 3);
    REQUIRE(result == 20 + 400 + 8902);
  }




//  REQUIRE(result == 124132536);
}
