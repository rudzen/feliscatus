#include "feliscatus.h"
#include "square.h"
#include "bitboard.h"
#include "magic.h"
#include "zobrist.h"
#include "util.h"
#include "pawnhashtable.h"
#include "transpositional.h"

constexpr int TT_ENTRY_SIZE   = 16;
constexpr int PAWN_ENTRY_SIZE = 16;

int main(/*int argc, char *argv[]*/) {
  util::check_size<HashEntry, TT_ENTRY_SIZE>();
  util::check_size<PawnHashEntry, PAWN_ENTRY_SIZE>();

  squares::init();
  bitboard::init();
  attacks::init();
  zobrist::init();

  Felis engine;
  engine.run();
}