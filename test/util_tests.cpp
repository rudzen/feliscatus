#define CATCH_CONFIG_MAIN

#include <catch2/catch.hpp>

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
