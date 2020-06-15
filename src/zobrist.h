#pragma once

#include <array>
#include "types.h"
#include "prng.h"

namespace zobrist {

inline Key zobrist_pst[14][64];
inline std::array<Key, 16> zobrist_castling{};
inline Key zobrist_side;
inline std::array<Key, 8> zobrist_ep_file{};

inline void init() {

  constexpr Key Seed = 1070372;

  PRNG<Key> rng(Seed);

  for (const auto p : PieceTypes)
  {
    for (const auto sq : Squares)
    {
      zobrist_pst[p][sq]     = rng();
      zobrist_pst[p + 8][sq] = rng();
    }
  }

  for (auto &i : zobrist_castling)
    i = rng();

  for (auto &i : zobrist_ep_file)
    i = rng();

  zobrist_side = rng();
}
}// namespace zobrist