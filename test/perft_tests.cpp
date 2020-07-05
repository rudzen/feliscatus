#define CATCH_CONFIG_MAIN

#include <catch2/catch.hpp>
#include "../src/transpositional.h"
#include "../src/bitboard.h"
#include "../src/magic.h"
#include "../src/perft.h"
#include "../src/board.h"
#include "../src/miscellaneous.h"
#include "../src/datapool.h"

TEST_CASE("Perft basic at depth 6", "[perft_basic]")
{
  TT.init(1);
  bitboard::init();
  attacks::init();
  Board::init();

  Pool.set(1);

  auto b = Board(start_position, Pool.main());

  auto result = perft::perft(&b, 6);

  REQUIRE(result == 124132536);
}