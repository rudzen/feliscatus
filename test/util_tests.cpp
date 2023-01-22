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

#include "../src/util.hpp"
#include "../src/types.hpp"

TEST_CASE("Abs test", "[abs]")
{

  constexpr auto expected = 42;

  constexpr auto actual_positive = util::abs(42);

  REQUIRE(actual_positive == expected);

  constexpr auto actial_negative = util::abs(-42);

  REQUIRE(actial_negative == expected);
}

TEST_CASE("in_between positive test", "[in_between_positive]")
{

  constexpr auto expected = true;
  constexpr auto pt       = KNIGHT;

  constexpr auto min_boundry = KNIGHT;
  constexpr auto max_boundry = ROOK;

  constexpr auto actual = util::in_between<min_boundry, max_boundry>(pt);

  REQUIRE(actual == expected);
}

TEST_CASE("in_between negative test", "[in_between_negative]")
{

  constexpr auto expected = false;
  constexpr auto pt       = KNIGHT;

  constexpr auto min_boundry = BISHOP;
  constexpr auto max_boundry = QUEEN;

  constexpr auto actual = util::in_between<min_boundry, max_boundry>(pt);

  REQUIRE(actual == expected);
}
