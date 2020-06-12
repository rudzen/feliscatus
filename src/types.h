#pragma once

#include <cstdint>

using Bitboard = uint64_t;

enum Color : uint8_t {
  WHITE, BLACK, COL_NB
};

enum HashNodeType : uint8_t {
  Void = 0, // TODO : rename
  EXACT = 1,
  BETA = 2,
  ALPHA = 4
};
