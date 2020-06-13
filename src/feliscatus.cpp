
#include "feliscatus.h"
#include "square.h"
#include "bitboard.h"
#include "magic.h"
#include "zobrist.h"

int main() {
  squares::init();
  bitboard::init();
  attacks::init();
  zobrist::init();

  Felis engine;
  engine.run();
}