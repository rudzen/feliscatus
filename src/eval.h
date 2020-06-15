#pragma once

class Game;
struct PawnHashTable;

namespace Eval {

int evaluate(Game *g, PawnHashTable *pawnTable, int alpha, int beta);

int tune(Game *g, PawnHashTable *pawnTable, int alpha, int beta);

}
