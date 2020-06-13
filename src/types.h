#pragma once

#include <cstdint>
#include <array>

using Bitboard = uint64_t;

enum Color : uint8_t {
  WHITE, BLACK, COL_NB
};

// Toggle color
constexpr Color operator~(const Color c) noexcept { return static_cast<Color>(c ^ BLACK); }

constexpr std::array<Color, COL_NB> Colors {WHITE, BLACK};

enum HashNodeType : uint8_t {
  Void = 0, // TODO : rename
  EXACT = 1,
  BETA = 2,
  ALPHA = 4
};
