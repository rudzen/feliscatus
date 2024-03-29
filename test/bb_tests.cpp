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

#include "../src/types.hpp"
#include "../src/bitboard.hpp"

TEST_CASE("single bit detection mto()", "[sing_bit_detection_mto]")
{
  constexpr auto sq            = make_square(FILE_A, RANK_2);
  constexpr auto sq2           = make_square(FILE_B, RANK_7);
  constexpr auto one           = bit(sq) | bit(sq2);
  constexpr auto expectedCount = more_than_one(one);

  REQUIRE(expectedCount == true);
  REQUIRE(std::has_single_bit(one) != true);
}

TEST_CASE("does msb yield correct square", "[msb]")
{
  constexpr auto expected  = make_square(FILE_B, RANK_5);
  constexpr auto secondary = make_square(FILE_A, RANK_1);
  constexpr auto bb        = bit(expected) | bit(secondary);
  constexpr auto actual    = msb(bb);

  REQUIRE(expected == actual);
}