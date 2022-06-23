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

#include <cstdint>
#include <catch2/catch.hpp>

constexpr std::uint64_t Factorial(const std::uint64_t number)
{
  return number <= 1 ? number : Factorial(number - 1) * number;
}

TEST_CASE("Factorials are computed with constexpr", "[factorial]")
{
  constexpr auto expected_1  = 1;
  constexpr auto expected_2  = 2;
  constexpr auto expected_3  = 6;
  constexpr auto expected_10 = 3628800;

  constexpr auto actual_1 = Factorial(1);
  STATIC_REQUIRE(actual_1 == expected_1);

  constexpr auto actual_2 = Factorial(2);
  STATIC_REQUIRE(actual_2 == expected_2);

  constexpr auto actual_3 = Factorial(3);
  STATIC_REQUIRE(actual_3 == expected_3);

  constexpr auto actual_10 = Factorial(10);
  STATIC_REQUIRE(actual_10 == expected_10);
}
