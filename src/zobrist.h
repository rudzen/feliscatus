#pragma once

#include <cstdint>
#include <random>
#include "piece.h"
#include "square.h"

namespace zobrist {

inline uint64_t zobrist_pst[14][64];
inline uint64_t zobrist_castling[16];
inline uint64_t zobrist_side;
inline uint64_t zobrist_ep_file[8];

inline void init() {
  std::mt19937_64 prng64;
  prng64.seed(std::mt19937_64::default_seed);

  for (auto p = Pawn; p <= King; p++)
  {
    for (const auto sq : Squares)
    {
      zobrist_pst[p][sq]     = prng64();
      zobrist_pst[p + 8][sq] = prng64();
    }
  }

  for (auto &i : zobrist_castling)
    i = prng64();

  for (auto &i : zobrist_ep_file)
    i = prng64();

  zobrist_side = prng64();
}

}// namespace zobrist