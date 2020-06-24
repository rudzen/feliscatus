#pragma once

#include <cstdint>

#include "types.h"

class Game;

namespace perft {

  uint64_t perft(Game *g, int depth = 6, int flags = LEGALMOVES);

  uint64_t divide(Game *g, int depth = 6, int flags = LEGALMOVES);

}
