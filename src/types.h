#pragma once

#include <cstdint>
#include <array>
#include <ranges>
#include <cstdio>
#include <string_view>

using Bitboard = uint64_t;
using Key      = uint64_t;

constexpr int MAXDEPTH = 128;

enum Square {
  a1, b1, c1, d1, e1, f1, g1, h1,
  a2, b2, c2, d2, e2, f2, g2, h2,
  a3, b3, c3, d3, e3, f3, g3, h3,
  a4, b4, c4, d4, e4, f4, g4, h4,
  a5, b5, c5, d5, e5, f5, g5, h5,
  a6, b6, c6, d6, e6, f6, g6, h6,
  a7, b7, c7, d7, e7, f7, g7, h7,
  a8, b8, c8, d8, e8, f8, g8, h8,
  no_square,
  sq_nb = 64
};

constexpr std::array<Square, sq_nb> Squares
  { a1, b1, c1, d1, e1, f1, g1, h1,
    a2, b2, c2, d2, e2, f2, g2, h2,
    a3, b3, c3, d3, e3, f3, g3, h3,
    a4, b4, c4, d4, e4, f4, g4, h4,
    a5, b5, c5, d5, e5, f5, g5, h5,
    a6, b6, c6, d6, e6, f6, g6, h6,
    a7, b7, c7, d7, e7, f7, g7, h7,
    a8, b8, c8, d8, e8, f8, g8, h8
  };

constexpr std::array<std::string_view, sq_nb> SquareString {
    "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1",
    "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
    "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
    "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
    "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
    "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
    "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
    "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8"
};

constexpr std::array<uint32_t, 2> oo_allowed_mask{1, 4};

constexpr std::array<uint32_t, 2> ooo_allowed_mask{2, 8};

inline std::array<Square, 2> oo_king_from{};
constexpr std::array<Square, 2> oo_king_to{g1, g8};

inline std::array<Square, 2> ooo_king_from{};
constexpr std::array<Square, 2> ooo_king_to{c1, c8};

/// indexed by the position of the king
inline std::array<Square, sq_nb> rook_castles_to{};

/// indexed by the position of the king
inline std::array<Square, sq_nb> rook_castles_from{};

constexpr std::string_view square_to_string(const Square sq) {
  return SquareString[sq];
}

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

constexpr int Pawn      = 0;
constexpr int Knight    = 1;
constexpr int Bishop    = 2;
constexpr int Rook      = 3;
constexpr int Queen     = 4;
constexpr int King      = 5;
constexpr int NoPiece   = 6;
constexpr int AllPieces = 7;

constexpr std::array<int, 6> PieceTypes{ Pawn, Knight, Bishop, Rook, Queen, King };

constexpr std::array<int, 6> piece_values{100, 400, 400, 600, 1200, 0};

constexpr int piece_value(const int p) {
  return piece_values[PieceTypes[p & 7]];
}

constexpr std::array<std::string_view, 6> piece_notation {" ", "n", "b", "r", "q", "k"};

constexpr std::string_view piece_to_string(const int piece) {
  return piece_notation[PieceTypes[piece]];
}

enum Move : uint32_t {
  MOVE_NONE = 0
};

enum MoveType : uint8_t {
  NORMAL     = 0,
  DOUBLEPUSH = 1,
  CASTLE     = 1 << 1,
  EPCAPTURE  = 1 << 2,
  PROMOTION  = 1 << 3,
  CAPTURE    = 1 << 4
};

// Move generation flags
constexpr int LEGALMOVES     = 1;
constexpr int STAGES         = 2;
constexpr int QUEENPROMOTION = 4;

#define ENABLE_BASE_OPERATORS_ON(T)                                \
constexpr T operator+(const T d1, const int d2) noexcept { return static_cast<T>(static_cast<int>(d1) + d2); } \
constexpr T operator-(const T d1, const int d2) noexcept { return static_cast<T>(static_cast<int>(d1) - d2); } \
constexpr T operator-(const T d) noexcept { return static_cast<T>(-static_cast<int>(d)); }                  \
constexpr inline T& operator+=(T& d1, const int d2) noexcept { return d1 = d1 + d2; }         \
constexpr inline T& operator-=(T& d1, const int d2) noexcept { return d1 = d1 - d2; }

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
ENABLE_INCR_OPERATORS_ON(Square)
ENABLE_INCR_OPERATORS_ON(File)
ENABLE_INCR_OPERATORS_ON(Rank)

#undef ENABLE_FULL_OPERATORS_ON
#undef ENABLE_INCR_OPERATORS_ON
#undef ENABLE_BASE_OPERATORS_ON

constexpr MoveType operator|(const MoveType mt, const int v) noexcept {
  return static_cast<MoveType>(static_cast<int>(mt) + static_cast<int>(v));
}

constexpr Square operator+(const Square s, const Direction d) noexcept {
  return static_cast<Square>(static_cast<int>(s) + static_cast<int>(d));
}

constexpr Square operator-(const Square s, const Direction d) noexcept {
  return static_cast<Square>(static_cast<int>(s) - static_cast<int>(d));
}

constexpr Square &operator+=(Square &s, const Direction d) noexcept { return s = s + d; }
constexpr Square &operator-=(Square &s, const Direction d) noexcept { return s = s - d; }

constexpr Rank rank_of(const Square sq) { return static_cast<Rank>(sq >> 3); }

constexpr File file_of(const Square sq) { return static_cast<File>(sq & 7); }

constexpr bool is_dark(const Square sq) { return ((9 * sq) & 8) == 0; }

constexpr bool same_color(const Square sq1, const Square sq2) { return is_dark(sq1) == is_dark(sq2); }

constexpr Square make_square(const File f, const Rank r) { return static_cast<Square>((r << 3) + f); }

constexpr Square relative_square(const Color c, const Square s) { return static_cast<Square>(s ^ (c * 56)); }

constexpr Rank relative_rank(const Color c, const Square s) { return relative_rank(c, rank_of(s)); }

constexpr int move_piece(const Move move) { return move >> 26 & 15; }

constexpr int move_captured(const Move move) { return move >> 22 & 15; }

constexpr int move_promoted(const Move move) { return move >> 18 & 15; }

constexpr MoveType move_type(const Move move) { return static_cast<MoveType>(move >> 12 & 63); }

constexpr Square move_from(const Move move) { return static_cast<Square>(move >> 6 & 63); }

constexpr Square move_to(const Move move) { return static_cast<Square>(move & 63); }

constexpr int move_piece_type(const Move move) { return move >> 26 & 7; }

constexpr Color move_side(const Move m) { return static_cast<Color>(m >> 29 & 1); }

constexpr int side_mask(const Move m) { return move_side(m) << 3; }

constexpr int is_capture(const Move m) { return move_type(m) & (CAPTURE | EPCAPTURE); }

constexpr int is_ep_capture(const Move m) { return move_type(m) & EPCAPTURE; }

constexpr int is_castle_move(const Move m) { return move_type(m) & CASTLE; }

constexpr int is_promotion(const Move m) { return move_type(m) & PROMOTION; }

constexpr bool is_queen_promotion(const Move m) { return is_promotion(m) && (move_promoted(m) & 7) == Queen; }

constexpr bool is_null_move(const Move m) { return m == 0; }

template<MoveType Type>
constexpr Move init_move(const int piece, const int captured, const Square from, const Square to, const int promoted) {
  return static_cast<Move>((piece << 26) | (captured << 22) | (promoted << 18) | (Type << 12) | (from << 6) | static_cast<int>(to));
}

constexpr Move init_move(const int piece, const int captured, const Square from, const Square to, const MoveType type, const int promoted) {
  return static_cast<Move>((piece << 26) | (captured << 22) | (promoted << 18) | (type << 12) | (from << 6) | static_cast<int>(to));
}
