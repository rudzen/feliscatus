#define CATCH_CONFIG_MAIN

#include <catch2/catch.hpp>

#include "../src/transpositional.hpp"
#include "../src/bitboard.hpp"
#include "../src/perft.hpp"
#include "../src/board.hpp"
#include "../src/miscellaneous.hpp"
#include "../src/tpool.hpp"

TEST_CASE("Perft basic at depth 6", "[perft_basic]")
{
  TT.init(1);
  bitboard::init();
  Board::init();

  pool.set(1);

  Board b{};
  b.set_fen(start_position, pool.main());

  auto result = perft::perft(&b, 6);

  REQUIRE(result == 124132536);
}