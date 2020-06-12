
#include "feliscatus.h"
#include "bitboard.h"
#include "magic.h"
#include "zobrist.h"
#include "square.h"

int main() {
  bitboard::init();
  attacks::init();
  zobrist::init();
  squares::init();

  Felis engine;
  engine.run();
}