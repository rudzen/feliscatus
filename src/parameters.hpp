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

#include <array>

#include "score.hpp"
#include "types.hpp"

namespace params
{

using S = Score;

// clang-format off

#if defined(TUNER)
inline int lazy_margin = 500;
#else
constexpr int lazy_margin = 500;
#endif

#if defined(TUNER)
inline int tempo = 10;
#else
constexpr int tempo = 10;
#endif

#if defined(TUNER)
inline std::array<int, PIECETYPE_NB> attacks_on_king
#else
constexpr std::array<int, PIECETYPE_NB> attacks_on_king
#endif
{
      0,      3,      5,   11,    39,    0
// Pawn  Knight  Bishop  Rook  Queen  King
};

#if defined(TUNER)
inline std::array<int, PIECETYPE_NB> piece_in_danger
#else
constexpr std::array<int, PIECETYPE_NB> piece_in_danger
#endif
{
      0,     41,     56,   34,    38,    0
// Pawn  Knight  Bishop  Rook  Queen  King
};

#if defined(TUNER)
inline std::array<Score, 14> bishop_mob2
#else
constexpr std::array<Score, 14> bishop_mob2
#endif
{
  S(-27, -60), S(-14, -27), S(-6, -20), S(-1, -10), S( 3, -8), S( 7, -2), S( 5, 2),
  S(  7,   3), S( 11,   4), S(15,   1), S(10,   6), S(18, 10), S(12, 16), S(29, 3)
};

#if defined(TUNER)
inline std::array<Score, 14> bishop_mob
#else
constexpr std::array<Score, 14> bishop_mob
#endif
{
  S(-23, -35), S(-17, -20), S(-13, -29), S(-13, -20), S(-6, -13), S(  0, -6), S( 5, -4),
  S(  5,   0), S( 10,   5), S(  3,   8), S(  2,  -1), S( 6,   5), S(-14,  7), S(16,  2)
};

#if defined(TUNER)
inline auto bishop_pair         = S(29, 57);
inline auto bishop_diagonal     = S(45,  0);
inline auto king_obstructs_rook = S(49, 9);
#else
constexpr auto bishop_pair         = S(29, 57);
constexpr auto bishop_diagonal     = S(45,  0);
constexpr auto king_obstructs_rook = S(49, 9);
#endif

#if defined(TUNER)
inline std::array<Score, 4> king_on_half_open
{ S( 12, 0), S( -9, 0), S(-55, 0), S( -97, 0) };
#else
constexpr std::array<Score, 4> king_on_half_open
{ S( 12, 0), S( -9, 0), S(-55, 0), S( -97, 0) };
#endif

#if defined(TUNER)
inline std::array<Score, 4> king_on_open
#else
constexpr std::array<Score, 4> king_on_open
#endif
{ S( 29, 0), S(-13, 0), S(-63, 0), S(-221, 0) };

#if defined(TUNER)
inline std::array<Score, 4> king_pawn_shelter
#else
constexpr std::array<Score, 4> king_pawn_shelter
#endif
{ S(-20, 0), S(  1, 0), S(  6, 0), S(  -7, 0) };

#if defined(TUNER)
inline std::array<Score, 9> knight_mob2
#else
constexpr std::array<Score, 9> knight_mob2
#endif
{
  S(-59, -137), S(-41, -56), S(-21, -38),
  S(-10,  -17), S(  1,  -7), S(  8,   0),
  S( 14,    0), S( 22,  -5), S( 24, -15)
};

#if defined(TUNER)
inline std::array<Score, 9> knight_mob
#else
constexpr std::array<Score, 9> knight_mob
#endif
{
  S(76, -40), S( 23,   8), S(  5, 14),
  S( 3,   0), S(  3,  -5), S( -2, -9),
  S(-6, -10), S(-14, -15), S(-16, -8)
};

#if defined(TUNER)
inline std::array<Score, 8> passed_pawn
#else
constexpr std::array<Score, 8> passed_pawn
#endif
{
  S(0, 0), S(-16, -16), S(-16, -7), S(-12, 2), S(23, 12), S(68, 13), S(95, -115), S(0, 0),
};

#if defined(TUNER)
inline std::array<Score, RANK_NB> passed_pawn_king_dist_them
#else
constexpr std::array<Score, RANK_NB> passed_pawn_king_dist_them
#endif
{ S(0, 0), S(0, -66), S(0, -24), S(0,  5), S(0,  22), S(0,  33), S(0,  39), S(0,  21)};

#if defined(TUNER)
inline std::array<Score, RANK_NB> passed_pawn_king_dist_us
#else
constexpr std::array<Score, RANK_NB> passed_pawn_king_dist_us
#endif
{ S(0, 0), S(0,  23), S(0,  24), S(0,  0), S(0, -10), S(0, -11), S(0,   0), S(0, -17)};

#if defined(TUNER)
inline std::array<Score, RANK_NB> passed_pawn_no_attacks
#else
constexpr std::array<Score, RANK_NB> passed_pawn_no_attacks
#endif
{ S(0, 0), S(0,   3), S(0,   3), S(0, 13), S(0,  24), S(0,  48), S(0,  85), S(0,   0)};

#if defined(TUNER)
inline std::array<Score, RANK_NB> passed_pawn_no_them
#else
constexpr std::array<Score, RANK_NB> passed_pawn_no_them
#endif
{ S(0, 0), S(0,   2), S(0,   6), S(0, 22), S(0,  36), S(0,  64), S(0, 129), S(0,   0)};

#if defined(TUNER)
inline std::array<Score, RANK_NB> passed_pawn_no_us
#else
constexpr std::array<Score, RANK_NB> passed_pawn_no_us
#endif
{ S(0, 0), S(0,   0), S(0,  -1), S(0,  3), S(0,  17), S(0,  32), S(0, 121), S(0,   0)};


#if defined(TUNER)
inline std::array<Score, 2> pawn_behind
#else
constexpr std::array<Score, 2> pawn_behind
#endif
{ S( -4,   4), S(-28,  -8)};

#if defined(TUNER)
inline std::array<Score, 2> pawn_doubled
#else
constexpr std::array<Score, 2> pawn_doubled
#endif
{ S(-15, -20), S(  1, -15)};

#if defined(TUNER)
inline std::array<Score, 2> pawn_isolated
#else
constexpr std::array<Score, 2> pawn_isolated
#endif
{ S(-13,  -7), S(-28, -32)};

#if defined(TUNER)
inline std::array<Score, 28> queen_mob
#else
constexpr std::array<Score, 28> queen_mob
#endif
{
  S(-19,  -3), S(-34, -30), S(-15, -17), S(-15, -27), S(-13, -36), S(-10, -37), S(-10, -30),
  S(-11, -24), S( -6, -27), S( -1, -21), S(  2, -17), S( -1, -11), S( -3,  -1), S(  2,   4),
  S(  3,   2), S(  5,   5), S(  7,   2), S(  3,   6), S(  9,   3), S( 13,   1), S( 25, -15),
  S( 34, -16), S( 33, -15), S( 58, -34), S( 44, -34), S( 41, -28), S( 14, -12), S( 19, -16)
};

#if defined(TUNER)
inline std::array<Score, 15> rook_mob
#else
constexpr std::array<Score, 15> rook_mob
#endif
{
  S(-28, -46), S(-20, -28), S(-13, -24), S(-16, -9), S(-17, -5),
  S(-10,  -2), S( -9,   5), S( -3,   9), S( -1, 14), S(  2, 19),
  S(  4,  21), S(  1,  27), S(  6,  27), S( 20, 15), S( 28,  3)
};

#if defined(TUNER)
inline int rook_open_file    = 13;
#else
constexpr int rook_open_file = 13;
#endif

using PcSqArr = std::array<Score, 64>;

#if defined(TUNER)

inline std::array<PcSqArr, PIECETYPE_NB> make_pst()
#else
consteval std::array<PcSqArr, PIECETYPE_NB> make_pst()
#endif
{

  constexpr std::array<Score, 64> pawn_pst
  {
    S(  0,  0), S(  0,  0), S(  0,  0), S(  0,  0), S(  0,  0), S( 0,  0), S( 0,  0), S(  0,  0),
    S(206,  0), S(124, 24), S( 96, 49), S(113, 29), S( 98, 34), S(36, 54), S( 9, 74), S( 13, 26),
    S( 19, 33), S( 25, 39), S( 43, 28), S( 36, 15), S( 58, 21), S(77, 13), S(64, 20), S( 30,  6),
    S( 10, 31), S( 16, 25), S(  9, 19), S( 25,  1), S( 23,  2), S(12, -2), S(-4,  9), S( -7, 13),
    S(  0, 18), S(  4, 18), S(  9, 12), S( 10,  5), S( 11,  4), S( 6,  8), S( 0,  5), S(-11,  2),
    S(  0, 13), S( -1,  9), S( -3, 10), S(  8,  9), S(  9, 12), S( 6,  7), S(18, -4), S( -4, -4),
    S( -7, 17), S(-11,  9), S(-14, 15), S(-11, 12), S(-14, 26), S(23, 11), S(29, -6), S(-16, -2),
    S(  0,  0), S(  0,  0), S(  0,  0), S(  0,  0), S(  0,  0), S( 0,  0), S( 0,  0), S(  0,  0)
  };

  constexpr std::array<Score, 64> knight_pst
  {
    S(-221,  -33), S(-103, -12), S(-59,   0), S(-11, -25), S( 38, -30), S(-31, -18), S(-103, -14), S(-255, -54),
    S( -66,  -31), S( -40, -16), S(  5, -15), S( 23, -14), S( 37,  -3), S( 35, -40), S( -11, -21), S( -32, -22),
    S( -22,  -37), S(  -3, -17), S( 42,  -3), S( 39,   3), S( 75, -12), S(115, -13), S(  10, -22), S( -33, -28),
    S( -23,  -32), S(   6, -11), S( 39,  -1), S( 66,  15), S( 19,  16), S( 61,   5), S(  11, -10), S(  25, -26),
    S( -25,  -28), S( -13, -10), S( 15,   3), S( 19,  10), S( 32,   9), S( 11,  10), S(  26, -11), S(  -6, -27),
    S( -39,  -54), S( -27, -28), S( -8, -19), S(  4,  -5), S( 20,  -5), S(  1, -24), S(  -4, -37), S( -40, -29),
    S( -46,  -60), S( -54, -37), S(-31, -40), S( -8, -34), S(-14, -37), S(-20, -34), S( -39, -29), S( -13, -48),
    S(-104, -159), S( -44, -75), S(-54, -46), S(-53, -30), S(-22, -46), S(-24, -52), S( -59, -67), S(  -2, -37)
  };

  constexpr std::array<Score, 64> bishop_pst
  {
    S(-65,   7), S(-58,  13), S(-27,  12), S(-41,  10), S(-46,  24), S(-70,  -5), S( 15, -19), S(-35, -14),
    S(-40,   2), S(-26,   9), S(-43,   3), S(-25,  16), S(-39,  -1), S(-10,  -7), S(-46,  -5), S(-59, -21),
    S( -5,  -8), S( -3,   7), S(  2,   4), S( 13, -17), S( 22,  12), S( 64,  -5), S( 29,   1), S(  4, -19),
    S(-27,  -7), S( -3,   3), S(  7,  -6), S( 41,   5), S( 14,   7), S( 14,   6), S( -5, -11), S(-24,  -3),
    S( -3, -18), S(-12, -16), S(  2,   7), S( 17,   2), S( 24,  -7), S( -3,  -6), S(  2, -13), S( -8, -22),
    S(-19, -23), S(  7, -19), S(  0, -15), S(  6,  -5), S(  2,   2), S(  5, -10), S(  4, -14), S(  9, -16),
    S(  7, -33), S(  0, -36), S( 12, -34), S(-13, -15), S(  1, -16), S( 16, -33), S( 25, -26), S( 16, -64),
    S( -5, -46), S( 20, -36), S(-23, -27), S(-23, -15), S(-14, -26), S(-14, -16), S( -8, -63), S(-23, -38)
  };

  constexpr std::array<Score, 64> rook_pst
  {
    S(  1,  46), S(  3,  47), S( 25,  33), S( 31,  28), S( 24,  36), S( 24,  42), S(35,  37), S( 54,  29),
    S(  3,  44), S( -8,  41), S( 28,  28), S( 40,  23), S( 39,  20), S( 56,  22), S(45,  20), S( 39,  16),
    S(-18,  49), S(  8,  27), S( 14,  28), S( 31,  10), S( 59,  12), S( 67,  21), S(51,  13), S( 11,  32),
    S(-41,  40), S(-23,  35), S(  4,  23), S( 20,  14), S( 16,  13), S( 17,  21), S(24,  20), S(-11,  18),
    S(-46,  28), S(-43,  34), S(-26,  23), S(-16,  11), S(-22,  10), S(-23,  17), S(-7,  11), S(-45,  19),
    S(-48,  16), S(-35,  14), S(-33,   1), S(-31,  -6), S(-10, -11), S(-21,  -1), S(-6,  -1), S(-25,   3),
    S(-45, -12), S(-33,  -8), S(-18, -12), S( -4, -24), S( -3, -23), S(-10, -19), S(-7, -21), S(-68,  -6),
    S(-16, -13), S( -9, -15), S(  0, -12), S(  3, -20), S(  7, -26), S(  4, -16), S( 8, -11), S(-18, -19)
  };

  constexpr std::array<Score, 64> queen_pst
  {
    S(-10,   5), S(  4,  14), S( 10,  10), S( 13,  23), S( 14,  20), S( 86,   2), S( 64,    9), S( 57,    8),
    S(-36,   5), S(-60,  40), S(-31,  40), S(-10,  38), S(-10,  49), S( 43,  25), S( 5,    25), S( 39,   10),
    S( -9, -22), S(-17,   7), S(-14,  38), S( -4,  37), S( 36,  40), S( 63,  20), S( 69,    2), S( 16,   12),
    S(-34, -10), S(-28,  -5), S(-30,  24), S(-20,  45), S(-16,  46), S( -6,  32), S(  7,   38), S( -8,    5),
    S(-22, -32), S(-17,  -5), S(-15,  -2), S(-16,  23), S(-10,  17), S(-13,   4), S(  4,   -3), S(-13,    3),
    S(-21, -50), S( -4, -42), S(-10, -20), S(-13, -36), S( -4, -18), S(  7, -24), S(  6,  -25), S( -9,  -39),
    S(-14, -73), S( -3, -47), S(  2, -64), S(  4, -61), S(  3, -57), S( 26, -97), S( 16, -108), S(-12,  -64),
    S(  8, -89), S( -8, -85), S( -5, -67), S(  4, -61), S(  5, -84), S(-18, -96), S(-58, -123), S( -9, -110)
  };

  constexpr std::array<Score, 64> king_pst
  {
    S(-83, -116), S(106, -11), S(122,  13), S(137, -34), S(123, -25), S( 72, 22), S( 44,  18), S(-34, -136),
    S( -2,  -45), S( 67,  40), S( 37,  26), S( 44,  16), S( 43,  17), S( 27, 36), S(-13,  51), S(-60,  -75),
    S(-44,   18), S( 47,  41), S(  7,  40), S( 17,  35), S( -2,  25), S( 12, 33), S( 16,  52), S(-47,   -5),
    S(-16,   -8), S( 52,  24), S( 21,  34), S(-12,  36), S( -1,  31), S(  5, 38), S( 30,  23), S(-69,   -4),
    S(-23,  -25), S( 25,   5), S(  6,  17), S(  7,  24), S(-15,  32), S(-15, 25), S(-14,   7), S(-63,  -20),
    S(-49,  -31), S( 16,  -5), S(-13,   8), S(-12,  15), S(-24,  25), S(-37, 21), S(-10,   2), S(-53,  -19),
    S(-12,  -22), S(-14,  -9), S(  6,  -2), S(-40,   5), S(-36,  12), S(-32, 13), S( 13,  -5), S(  0,  -27),
    S(-52,  -44), S( 16, -35), S(  0, -16), S(-73,   2), S(-34, -20), S(-50, -8), S( 32, -38), S(  7,  -66)
  };

  std::array<PcSqArr, PIECETYPE_NB> result = {
    pawn_pst,
    knight_pst,
    bishop_pst,
    rook_pst,
    queen_pst,
    king_pst,
    {},
    {}
  };

  return result;

}

// clang-format on

#if defined(TUNER)

inline std::array<PcSqArr, PIECETYPE_NB> pc_sq_arr{};

inline void init()
{
  pc_sq_arr = make_pst();
}

#else

constexpr std::array<PcSqArr, PIECETYPE_NB> pc_sq_arr = make_pst();

#endif

template<PieceType Pt>
[[nodiscard]]
#if defined(TUNER)
inline Score &pst(const Square sq)
#else
constexpr Score pst(const Square sq)
#endif
{
  return pc_sq_arr[Pt][sq];
}


}
