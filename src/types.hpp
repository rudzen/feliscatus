/*
  Feliscatus, a UCI chess playing engine derived from Tomcat 1.0 (Bobcat 8.0)
  Copyright (C) 2008-2016 Gunnar Harms (Bobcat author)
  Copyright (C) 2017      FireFather (Tomcat author)
  Copyright (C) 2020-2022 Rudy Alex Kohn

  Feliscatus is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Feliscatus is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <cstdint>
#include <array>
#include <ranges>
#include <cstdio>
#include <string_view>

#include "util.hpp"

using Bitboard = std::uint64_t;
using Key      = std::uint64_t;

constexpr int MAXDEPTH = 128;

enum Square
{
  A1, B1, C1, D1, E1, F1, G1, H1,
  A2, B2, C2, D2, E2, F2, G2, H2,
  A3, B3, C3, D3, E3, F3, G3, H3,
  A4, B4, C4, D4, E4, F4, G4, H4,
  A5, B5, C5, D5, E5, F5, G5, H5,
  A6, B6, C6, D6, E6, F6, G6, H6,
  A7, B7, C7, D7, E7, F7, G7, H7,
  A8, B8, C8, D8, E8, F8, G8, H8,
  NO_SQ,
  SQ_NB = 64
};

template<typename T>
concept SquareT = std::is_convertible_v<T, Square>;

constexpr std::array<Square, SQ_NB> Squares{A1, B1, C1, D1, E1, F1, G1, H1, A2, B2, C2, D2, E2, F2, G2, H2,
                                            A3, B3, C3, D3, E3, F3, G3, H3, A4, B4, C4, D4, E4, F4, G4, H4,
                                            A5, B5, C5, D5, E5, F5, G5, H5, A6, B6, C6, D6, E6, F6, G6, H6,
                                            A7, B7, C7, D7, E7, F7, G7, H7, A8, B8, C8, D8, E8, F8, G8, H8};

constexpr std::array<std::string_view, SQ_NB> SquareString{
  "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1", "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
  "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3", "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
  "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5", "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
  "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7", "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8"};

[[nodiscard]]
constexpr std::string_view square_to_string(const Square s)
{
  return SquareString[s];
}

enum Color : std::uint8_t
{
  WHITE,
  BLACK,
  COL_NB
};

template<typename T>
concept ColorT = std::is_convertible_v<T, Color>;

// Toggle color
template<ColorT C>
constexpr Color operator~(const C c) noexcept
{
  return static_cast<Color>(c ^ BLACK);
}

constexpr std::array<Color, COL_NB> Colors{WHITE, BLACK};

enum NodeType : std::uint8_t
{
  NO_NT = 0,
  EXACT = 1,
  BETA  = 2,
  ALPHA = 4
};

enum File : int
{
  FILE_A,
  FILE_B,
  FILE_C,
  FILE_D,
  FILE_E,
  FILE_F,
  FILE_G,
  FILE_H,
  FILE_NB
};

template<typename T>
concept FileT = std::is_convertible_v<T, File>;

constexpr std::array<File, 8> Files{FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H};

constexpr std::ranges::reverse_view ReverseFiles{Files};

enum Rank : int
{
  RANK_1,
  RANK_2,
  RANK_3,
  RANK_4,
  RANK_5,
  RANK_6,
  RANK_7,
  RANK_8,
  RANK_NB
};

template<typename T>
concept RankT = std::is_convertible_v<T, Rank>;

constexpr std::array<Rank, 8> Ranks{RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8};

constexpr std::ranges::reverse_view ReverseRanks{Ranks};

template<ColorT C, RankT R>
[[nodiscard]]
constexpr Rank relative_rank(const C c, const R r)
{
  return static_cast<Rank>(r ^ (c * 7));
}

enum Direction : int
{
  NORTH = 8,
  EAST  = 1,
  SOUTH = -NORTH,
  WEST  = -EAST,

  NORTH_EAST       = NORTH + EAST,           // 9
  SOUTH_EAST       = SOUTH + EAST,           // -7
  SOUTH_WEST       = SOUTH + WEST,           // -9
  NORTH_WEST       = NORTH + WEST,           // 7
  NORTH_NORTH_WEST = NORTH + NORTH + WEST,   // 15
  NORTH_NORTH_EAST = NORTH + NORTH + EAST,   // 17
  SOUTH_SOUTH_WEST = SOUTH + SOUTH + WEST,   // -17
  SOUTH_SOUTH_EAST = SOUTH + SOUTH + EAST,   // -15

  NO_DIRECTION = 0
};

template<typename T>
concept DirectionT = std::is_convertible_v<T, Direction>;

template<ColorT C>
[[nodiscard]]
constexpr Direction pawn_push(const C c)
{
  return c == WHITE ? NORTH : SOUTH;
}

enum PieceType
{
  PAWN            = 0,
  KNIGHT          = 1,
  BISHOP          = 2,
  ROOK            = 3,
  QUEEN           = 4,
  KING            = 5,
  NO_PT           = 6,
  ALL_PIECE_TYPES = 6,
  PIECETYPE_NB    = 8
};

template<typename T>
concept PieceTypeT = std::is_convertible_v<T, PieceType>;

constexpr std::array<PieceType, 6> PieceTypes{PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING};

enum Piece
{
  W_PAWN   = 0, W_KNIGHT =  1, W_BISHOP =  2, W_ROOK =  3, W_QUEEN =  4, W_KING =  5,
  B_PAWN   = 8, B_KNIGHT =  9, B_BISHOP = 10, B_ROOK = 11, B_QUEEN = 12, B_KING = 13,
  NO_PIECE = 6, PIECE_NB = 16
};

template<typename T>
concept PieceT = std::is_convertible_v<T, Piece>;

constexpr std::array<Piece, PieceTypes.size() * 2> Pieces{W_PAWN, W_KNIGHT, W_BISHOP, W_ROOK, W_QUEEN, W_KING,
                                                          B_PAWN, B_KNIGHT, B_BISHOP, B_ROOK, B_QUEEN, B_KING};

constexpr std::array<PieceType, 4> PromotionPieceTypes{QUEEN, ROOK, BISHOP, KNIGHT};

constexpr std::array<int, 6> piece_values{100, 400, 400, 600, 1200, 0};

constexpr std::array<std::string_view, 6> piece_notation{" ", "n", "b", "r", "q", "k"};

[[nodiscard]]
constexpr std::string_view piece_to_string(const PieceType pt)
{
  return piece_notation[pt];
}

[[nodiscard]]
constexpr PieceType type_of(const Piece pc)
{
  return static_cast<PieceType>(pc & 7);
}

template<ColorT C>
[[nodiscard]]
constexpr Piece make_piece(const PieceType pt, const C c)
{
  return static_cast<Piece>(pt | (c << 3));
}

[[nodiscard]]
constexpr int piece_value(const PieceType pt)
{
  return piece_values[pt];
}

[[nodiscard]]
constexpr int piece_value(const Piece pc)
{
  return piece_values[type_of(pc)];
}

template<PieceType Pt>
[[nodiscard]]
constexpr int piece_value() {
  return piece_values[Pt];
}

enum Move : std::uint32_t
{
  MOVE_NONE = 0
};

template<typename T>
concept MoveT = std::is_convertible_v<T, Move>;

enum MoveType : std::uint8_t
{
  NORMAL     = 0,
  DOUBLEPUSH = 1,
  CASTLE     = 1 << 1,
  EPCAPTURE  = 1 << 2,
  PROMOTION  = 1 << 3,
  CAPTURE    = 1 << 4
};

enum CastlingRight
{
  NO_CASTLING       = 0,
  WHITE_OO          = 1,
  WHITE_OOO         = WHITE_OO << 1,
  BLACK_OO          = WHITE_OO << 2,
  BLACK_OOO         = WHITE_OO << 3,
  KING_SIDE         = WHITE_OO | BLACK_OO,
  QUEEN_SIDE        = WHITE_OOO | BLACK_OOO,
  WHITE_ANY         = WHITE_OO | WHITE_OOO,
  BLACK_ANY         = BLACK_OO | BLACK_OOO,
  ANY_CASTLING      = KING_SIDE | QUEEN_SIDE,
  CASTLING_RIGHT_NB = 16
};

constexpr std::array<CastlingRight, 2> oo_allowed_mask{WHITE_OO, BLACK_OO};

constexpr std::array<CastlingRight, 2> ooo_allowed_mask{WHITE_OOO, BLACK_OOO};

constexpr std::array<Square, FILE_NB> CastlelingSquaresKing{H1, G1, F1, E1, D1, C1, B1, A1};

constexpr std::ranges::reverse_view CastlelingSquaresQueen{CastlelingSquaresKing};

constexpr std::array<Square, 2> oo_king_to{G1, G8};

constexpr std::array<Square, 2> ooo_king_to{C1, C8};

template<Color C, CastlingRight S>
[[nodiscard]]
constexpr CastlingRight make_castling()
{
  constexpr auto WHITE_CR = S == QUEEN_SIDE ? WHITE_OOO : WHITE_OO;
  constexpr auto BLACK_CR = S == QUEEN_SIDE ? BLACK_OOO : BLACK_OO;
  return C == WHITE ? WHITE_CR : BLACK_CR;
}

template<CastlingRight S, ColorT C>
[[nodiscard]]
constexpr CastlingRight make_castling(const C c)
{
  constexpr auto WHITE_CR = S == QUEEN_SIDE ? WHITE_OOO : WHITE_OO;
  constexpr auto BLACK_CR = S == QUEEN_SIDE ? BLACK_OOO : BLACK_OO;
  return c == WHITE ? WHITE_CR : BLACK_CR;
}

template<ColorT C>
[[nodiscard]]
constexpr CastlingRight operator&(const C c, const CastlingRight cr)
{
  return static_cast<CastlingRight>((c == WHITE ? WHITE_ANY : BLACK_ANY) & cr);
}

enum MoveGenFlags
{
  NONE       = 0,
  LEGALMOVES = 1,
  STAGES     = 1 << 1,
  CAPTURES   = 1 << 2,
  QUIET      = 1 << 3
};

template<typename T>
concept Okayable = PieceT<T>
                  || SquareT<T>
                  || PieceTypeT<T>
                  || MoveT<T>;

template<typename T>
concept Colorable = PieceT<T> || SquareT<T>;


/// color_of() determin color of a square or a piece

template<Colorable T>
[[nodiscard]]
constexpr Color color_of(const T t)
{
  if constexpr (std::is_same_v<T, Piece>)
    return static_cast<Color>(t >> 3);
  else if constexpr (std::is_same_v<T, Square>)
    return static_cast<Color>(((t ^ (t >> 3)) & 1) ^ 1);
  else
    return COL_NB;
}

#define ENABLE_BASE_OPERATORS_ON(T)                                                                                    \
  constexpr T operator+(const T d1, const int d2) noexcept                                                             \
  {                                                                                                                    \
    return static_cast<T>(static_cast<int>(d1) + d2);                                                                  \
  }                                                                                                                    \
  constexpr T operator-(const T d1, const int d2) noexcept                                                             \
  {                                                                                                                    \
    return static_cast<T>(static_cast<int>(d1) - d2);                                                                  \
  }                                                                                                                    \
  constexpr T operator-(const T d) noexcept                                                                            \
  {                                                                                                                    \
    return static_cast<T>(-static_cast<int>(d));                                                                       \
  }                                                                                                                    \
  constexpr inline T &operator+=(T &d1, const int d2) noexcept                                                         \
  {                                                                                                                    \
    return d1 = d1 + d2;                                                                                               \
  }                                                                                                                    \
  constexpr inline T &operator-=(T &d1, const int d2) noexcept                                                         \
  {                                                                                                                    \
    return d1 = d1 - d2;                                                                                               \
  }

#define ENABLE_INCR_OPERATORS_ON(T)                                                                                    \
  constexpr inline T &operator++(T &d) noexcept                                                                        \
  {                                                                                                                    \
    return d = static_cast<T>(static_cast<int>(d) + 1);                                                                \
  }                                                                                                                    \
  constexpr inline T &operator--(T &d) noexcept                                                                        \
  {                                                                                                                    \
    return d = static_cast<T>(static_cast<int>(d) - 1);                                                                \
  }

#define ENABLE_FULL_OPERATORS_ON(T)                                                                                    \
  ENABLE_BASE_OPERATORS_ON(T)                                                                                          \
  constexpr T operator*(const int i, const T d) noexcept                                                               \
  {                                                                                                                    \
    return static_cast<T>(i * static_cast<int>(d));                                                                    \
  }                                                                                                                    \
  constexpr T operator*(const T d, const int i) noexcept                                                               \
  {                                                                                                                    \
    return static_cast<T>(static_cast<int>(d) * i);                                                                    \
  }                                                                                                                    \
  constexpr T operator/(const T d, const int i) noexcept                                                               \
  {                                                                                                                    \
    return static_cast<T>(static_cast<int>(d) / i);                                                                    \
  }                                                                                                                    \
  constexpr int operator/(const T d1, const T d2) noexcept                                                             \
  {                                                                                                                    \
    return static_cast<int>(d1) / static_cast<int>(d2);                                                                \
  }                                                                                                                    \
  constexpr inline T &operator*=(T &d, const int i) noexcept                                                           \
  {                                                                                                                    \
    return d = static_cast<T>(static_cast<int>(d) * i);                                                                \
  }                                                                                                                    \
  constexpr inline T &operator/=(T &d, const int i) noexcept                                                           \
  {                                                                                                                    \
    return d = static_cast<T>(static_cast<int>(d) / i);                                                                \
  }

ENABLE_FULL_OPERATORS_ON(Direction)
ENABLE_INCR_OPERATORS_ON(Square)
ENABLE_INCR_OPERATORS_ON(File)
ENABLE_INCR_OPERATORS_ON(Rank)
ENABLE_INCR_OPERATORS_ON(PieceType)

#undef ENABLE_FULL_OPERATORS_ON
#undef ENABLE_INCR_OPERATORS_ON
#undef ENABLE_BASE_OPERATORS_ON

[[nodiscard]]
constexpr MoveType operator|(const MoveType mt, const int v) noexcept
{
  return static_cast<MoveType>(static_cast<int>(mt) + v);
}

[[nodiscard]]
constexpr Square operator+(const Square s, const Direction d) noexcept
{
  return static_cast<Square>(static_cast<int>(s) + static_cast<int>(d));
}

[[nodiscard]]
constexpr Square operator-(const Square s, const Direction d) noexcept
{
  return static_cast<Square>(static_cast<int>(s) - static_cast<int>(d));
}

constexpr Square &operator+=(Square &s, const Direction d) noexcept
{
  return s = s + d;
}

[[nodiscard]]
constexpr Square &operator-=(Square &s, const Direction d) noexcept
{
  return s = s - d;
}

template<SquareT S>
[[nodiscard]]
constexpr Rank rank_of(const S s)
{
  return static_cast<Rank>(s >> 3);
}

template<SquareT S>
[[nodiscard]]
constexpr File file_of(const S s)
{
  return static_cast<File>(s & 7);
}

template<SquareT S>
[[nodiscard]]
constexpr bool is_dark(const S s)
{
  return ((9 * s) & 8) == 0;
}

template<SquareT S>
[[nodiscard]]
constexpr bool same_color(const S s1, const S s2)
{
  return is_dark(s1) == is_dark(s2);
}

template<FileT F, RankT R>
[[nodiscard]]
constexpr Square make_square(const F f, const R r)
{
  return static_cast<Square>((r << 3) + f);
}

template<ColorT C, SquareT S>
[[nodiscard]]
constexpr Square relative_square(const C c, const S s)
{
  return static_cast<Square>(s ^ (c * 56));
}

template<ColorT C, SquareT S>
[[nodiscard]]
constexpr Rank relative_rank(const C c, const S s)
{
  return relative_rank(c, rank_of(s));
}

template<MoveT M>
[[nodiscard]]
constexpr Piece move_piece(const M m)
{
  return static_cast<Piece>(m >> 26 & 15);
}

template<MoveT M>
[[nodiscard]]
constexpr Piece move_captured(const M m)
{
  return static_cast<Piece>(m >> 22 & 15);
}

template<MoveT M>
[[nodiscard]]
constexpr Piece move_promoted(const M m)
{
  return static_cast<Piece>(m >> 18 & 15);
}

template<MoveT M>
[[nodiscard]]
constexpr MoveType type_of(const M m)
{
  return static_cast<MoveType>(m >> 12 & 63);
}

template<MoveT M>
[[nodiscard]]
constexpr Square move_from(const M m)
{
  return static_cast<Square>(m >> 6 & 63);
}

template<MoveT M>
[[nodiscard]]
constexpr Square move_to(const M m)
{
  return static_cast<Square>(m & 63);
}

template<MoveT M>
[[nodiscard]]
constexpr PieceType move_piece_type(const M m)
{
  return type_of(move_piece(m));
}

template<MoveT M>
[[nodiscard]]
constexpr Color move_side(const M m)
{
  return static_cast<Color>(m >> 29 & 1);
}

template<MoveT M>
[[nodiscard]]
constexpr bool is_capture(const M m)
{
  return type_of(m) & (CAPTURE | EPCAPTURE);
}

template<MoveT M>
[[nodiscard]]
constexpr bool is_ep_capture(const M m)
{
  return type_of(m) & EPCAPTURE;
}

template<MoveT M>
[[nodiscard]]
constexpr bool is_castle_move(const M m)
{
  return type_of(m) & CASTLE;
}

template<MoveT M>
[[nodiscard]]
constexpr bool is_promotion(const M m)
{
  return type_of(m) & PROMOTION;
}

template<MoveT M>
[[nodiscard]]
constexpr bool is_queen_promotion(const M m)
{
  return is_promotion(m) && type_of(move_promoted(m)) == QUEEN;
}

template<MoveType Mt, PieceT P, SquareT S>
[[nodiscard]]
constexpr Move init_move(
  const P pc,
  const P cap,
  const S from,
  const S to,
  const P promoted)
{
  return static_cast<Move>(
    (pc << 26) | (cap << 22) | (promoted << 18) | (Mt << 12) | (from << 6) | static_cast<int>(to));
}

template<PieceT P, SquareT S>
[[nodiscard]]
constexpr Move init_move(
  const P pc,
  const P captured,
  const S from,
  const S to,
  const MoveType mt,
  const P promoted)
{
  return static_cast<Move>(
    (pc << 26) | (captured << 22) | (promoted << 18) | (mt << 12) | (from << 6) | static_cast<int>(to));
}

/// Checks if Piece, PieceType, Square or Move is ok

template<Okayable T>
[[nodiscard]]
constexpr bool is_ok(const T t)
{
  if constexpr (std::is_same_v<T, Piece>)
    return is_ok(type_of(t));
  else if constexpr (std::is_same_v<T, Square>)
    return util::in_between<A1, H8>(t);
  else if constexpr (std::is_same_v<T, PieceType>)
    return util::in_between<PAWN, KING>(type_of(t));
  else if constexpr (std::is_same_v<T, Move>)
    return t != MOVE_NONE && move_from(t) != move_to(t);
}
