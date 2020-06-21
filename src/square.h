#pragma once

#include "types.h"

namespace squares {

inline void init() {
  for (const auto sq1 : Squares)
  {
    for (const auto sq2 : Squares)
    {
      const int ranks = std::abs(rank_of(sq1) - rank_of(sq2));
      const int files = std::abs(file_of(sq1) - file_of(sq2));
      dist[sq1][sq2]       = std::max(ranks, files);
    }
  }

  for (const auto side : Colors)
  {
    const auto rank_1 = relative_rank(side, RANK_1);
    rook_castles_to[make_square(FILE_G, rank_1)] = make_square(FILE_F, rank_1);
    rook_castles_to[make_square(FILE_C, rank_1)] = make_square(FILE_D, rank_1);
  }

  // Arrays castle_right_mask, rook_castles_from, ooo_king_from and oo_king_from
  // are initd in method setupCastling of class Game.
}

}// namespace squares

using namespace squares;