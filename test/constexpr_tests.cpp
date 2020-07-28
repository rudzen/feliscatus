#include <cstdint>
#include <catch2/catch.hpp>

constexpr uint64_t Factorial(const uint64_t number)
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
