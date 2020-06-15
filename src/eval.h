#pragma once

#include <array>
#include "game.h"
#include "hash.h"
#include "position.h"
#include "parameters.h"
#include "bitboard.h"
#include "magic.h"


namespace Eval {

int evaluate(const Game &game, PawnHashTable *pawnTable, const int alpha, const int beta);

int tune(const Game &game, PawnHashTable *pawnTable, const int alpha, const int beta);

}
