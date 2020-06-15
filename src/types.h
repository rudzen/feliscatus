#pragma once

#include <cstdint>
#include <array>
#include <ranges>
#include <cstdio>
#include <string_view>

using Bitboard = uint64_t;
using Key      = uint64_t;

enum Color : uint8_t {
  WHITE, BLACK, COL_NB
};

// Toggle color
constexpr Color operator~(const Color c) noexcept { return static_cast<Color>(c ^ BLACK); }

constexpr std::array<Color, COL_NB> Colors {WHITE, BLACK};

enum NodeType : uint8_t {
  Void = 0, // TODO : rename
  EXACT = 1,
  BETA = 2,
  ALPHA = 4
};

enum File : int { FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H, FILE_NB };

constexpr std::array<File, 8> Files{FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H};

constexpr std::ranges::reverse_view ReverseFiles {Files};

enum Rank : int { RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, RANK_NB };

constexpr std::array<Rank, 8> Ranks{RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8};

constexpr std::ranges::reverse_view ReverseRanks {Ranks};

constexpr Rank relative_rank(const Color c, const Rank r) { return static_cast<Rank>(r ^ (c * 7)); }


enum Direction : int {
  NORTH = 8,
  EAST = 1,
  SOUTH = -NORTH,
  WEST = -EAST,

  NORTH_EAST = NORTH + EAST,// 9
  SOUTH_EAST = SOUTH + EAST,// -7
  SOUTH_WEST = SOUTH + WEST,// -9
  NORTH_WEST = NORTH + WEST,// 7
  NORTH_NORTH_WEST = NORTH + NORTH + WEST,// 15
  NORTH_NORTH_EAST = NORTH + NORTH + EAST,// 17
  SOUTH_SOUTH_WEST = SOUTH + SOUTH + WEST,// -17
  SOUTH_SOUTH_EAST = SOUTH + SOUTH + EAST,// -15

  NO_DIRECTION = 0
};

constexpr Direction pawn_push(const Color c) { return c == WHITE ? NORTH : SOUTH; }

constexpr int Pawn    = 0;
constexpr int Knight  = 1;
constexpr int Bishop  = 2;
constexpr int Rook    = 3;
constexpr int Queen   = 4;
constexpr int King    = 5;
constexpr int NoPiece = 6;

constexpr std::array<int, 6> PieceTypes{ Pawn, Knight, Bishop, Rook, Queen, King };

constexpr std::array<int, 6> piece_values{100, 400, 400, 600, 1200, 0};

constexpr int piece_value(const int p) {
  return piece_values[p & 7];
}

constexpr std::array<std::string_view, 6> piece_notation {" ", "n", "b", "r", "q", "k"};

constexpr std::string_view piece_to_string(const int piece) {
  return piece_notation[piece];
}

// Move generation flags
constexpr int LEGALMOVES     = 1;
constexpr int STAGES         = 2;
constexpr int QUEENPROMOTION = 4;

#define ENABLE_BASE_OPERATORS_ON(T)                                \
constexpr T operator+(const T d1, const T d2) noexcept { return static_cast<T>(static_cast<int>(d1) + static_cast<int>(d2)); } \
constexpr T operator-(const T d1, const T d2) noexcept { return static_cast<T>(static_cast<int>(d1) - static_cast<int>(d2)); } \
constexpr T operator-(const T d) noexcept { return static_cast<T>(-static_cast<int>(d)); }                  \
constexpr inline T& operator+=(T& d1, const T d2) noexcept { return d1 = d1 + d2; }         \
constexpr inline T& operator-=(T& d1, const T d2) noexcept { return d1 = d1 - d2; }

#define ENABLE_INCR_OPERATORS_ON(T)                                                                     \
  constexpr inline T &operator++(T &d) noexcept { return d = static_cast<T>(static_cast<int>(d) + 1); } \
  constexpr inline T &operator--(T &d) noexcept { return d = static_cast<T>(static_cast<int>(d) - 1); }

#define ENABLE_FULL_OPERATORS_ON(T)                                                                                  \
  ENABLE_BASE_OPERATORS_ON(T)                                                                                        \
  constexpr T operator*(const int i, const T d) noexcept { return static_cast<T>(i * static_cast<int>(d)); }         \
  constexpr T operator*(const T d, const int i) noexcept { return static_cast<T>(static_cast<int>(d) * i); }         \
  constexpr T operator/(const T d, const int i) noexcept { return static_cast<T>(static_cast<int>(d) / i); }         \
  constexpr int operator/(const T d1, const T d2) noexcept { return static_cast<int>(d1) / static_cast<int>(d2); }   \
  constexpr inline T &operator*=(T &d, const int i) noexcept { return d = static_cast<T>(static_cast<int>(d) * i); } \
  constexpr inline T &operator/=(T &d, const int i) noexcept { return d = static_cast<T>(static_cast<int>(d) / i); }

ENABLE_FULL_OPERATORS_ON(Direction)
ENABLE_INCR_OPERATORS_ON(File)
ENABLE_INCR_OPERATORS_ON(Rank)

#undef ENABLE_FULL_OPERATORS_ON
#undef ENABLE_INCR_OPERATORS_ON
#undef ENABLE_BASE_OPERATORS_ON
