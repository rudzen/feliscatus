/*
  Feliscatus, a UCI chess playing engine derived from Tomcat 1.0 (Bobcat 8.0)
  Copyright (C) 2008-2016 Gunnar Harms (Bobcat author)
  Copyright (C) 2017      FireFather (Tomcat author)
  Copyright (C) 2020      Rudy Alex Kohn

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

#include "score.h"
#include "types.h"

// TODO : Join eg and mg values into a single int (or simular)

// clang-format off

inline int lazy_margin = 500;

inline int tempo = 10;

inline std::array<int, PieceType_Nb> attacks_on_king
{
      0,      3,      5,   11,    39,    0
// Pawn  Knight  Bishop  Rook  Queen  King
};

inline std::array<int, PieceType_Nb> piece_in_danger
{
      0,     41,     56,   34,    38,    0
// Pawn  Knight  Bishop  Rook  Queen  King
};

inline std::array<Score, 14> bishop_mob2 {
  Score(-27, -60), Score(-14, -27), Score(-6, -20), Score(-1, -10), Score( 3, -8), Score( 7, -2), Score( 5, 2),
  Score(  7,   3), Score( 11,   4), Score(15,   1), Score(10,   6), Score(18, 10), Score(12, 16), Score(29, 3)
};

inline std::array<Score, 14> bishop_mob {
  Score(-23, -35), Score(-17, -20), Score(-13, -29), Score(-13, -20), Score(-6, -13), Score(  0, -6), Score( 5, -4),
  Score(  5,   0), Score( 10,   5), Score(  3,   8), Score(  2,  -1), Score( 6,   5), Score(-14,  7), Score(16,  2)
};

inline Score bishop_pair         = Score(29, 57);
inline Score bishop_diagonal     = Score(45,  0);
inline Score king_obstructs_rook = Score(49, 9);

inline std::array<int, 4> king_on_half_open{12, -9, -55, -97};
inline std::array<int, 4> king_on_open     {29, -13, -63, -221};
inline std::array<int, 4> king_pawn_shelter{-20, 1, 6, -7};

inline std::array<Score, 9> knight_mob2 {
  Score(-59, -137), Score(-41, -56), Score(-21, -38),
  Score(-10,  -17), Score(  1,  -7), Score(  8,   0),
  Score( 14,    0), Score( 22,  -5), Score( 24, -15)
};

inline std::array<Score, 9> knight_mob {
  Score(76, -40), Score( 23,   8), Score(  5, 14),
  Score( 3,   0), Score(  3,  -5), Score( -2, -9),
  Score(-6, -10), Score(-14, -15), Score(-16, -8)
};

inline std::array<Score, 8> passed_pawn {
  Score(0, 0), Score(-16, -16), Score(-16, -7), Score(-12, 2), Score(23, 12), Score(68, 13), Score(95, -115), Score(0, 0),
};

inline std::array<int, 8> passed_pawn_king_dist_them {0, -66, -24,  5,  22,  33,  39,  21};
inline std::array<int, 8> passed_pawn_king_dist_us   {0,  23,  24,  0, -10, -11,   0, -17};
inline std::array<int, 8> passed_pawn_no_attacks     {0,   3,   3, 13,  24,  48,  85,   0};
inline std::array<int, 8> passed_pawn_no_them        {0,   2,   6, 22,  36,  64, 129,   0};
inline std::array<int, 8> passed_pawn_no_us          {0,   0,  -1,  3,  17,  32, 121,   0};

inline std::array<Score, 2> pawn_behind   { Score( -4,   4), Score(-28,  -8)};
inline std::array<Score, 2> pawn_doubled  { Score(-15, -20), Score(  1, -15)};
inline std::array<Score, 2> pawn_isolated { Score(-13,  -7), Score(-28, -32)};

inline int queen_attack_king = 39;

inline std::array<Score, 28> queen_mob {
  Score(-19,  -3), Score(-34, -30), Score(-15, -17), Score(-15, -27), Score(-13, -36), Score(-10, -37), Score(-10, -30),
  Score(-11, -24), Score( -6, -27), Score( -1, -21), Score(  2, -17), Score( -1, -11), Score( -3,  -1), Score(  2,   4),
  Score(  3,   2), Score(  5,   5), Score(  7,   2), Score(  3,   6), Score(  9,   3), Score( 13,   1), Score( 25, -15),
  Score( 34, -16), Score( 33, -15), Score( 58, -34), Score( 44, -34), Score( 41, -28), Score( 14, -12), Score( 19, -16)
};

inline int rook_attack_king  = 11;

inline std::array<Score, 15> rook_mob {
  Score(-28, -46), Score(-20, -28), Score(-13, -24), Score(-16, -9), Score(-17, -5),
  Score(-10,  -2), Score( -9,   5), Score( -3,   9), Score( -1, 14), Score(  2, 19),
  Score(  4,  21), Score(  1,  27), Score(  6,  27), Score( 20, 15), Score( 28,  3)
};

inline int rook_open_file    = 13;

inline std::array<Score, 64> bishop_pst {
  Score(-65,   7), Score(-58,  13), Score(-27,  12), Score(-41,  10), Score(-46,  24), Score(-70,  -5), Score( 15, -19), Score(-35, -14),
  Score(-40,   2), Score(-26,   9), Score(-43,   3), Score(-25,  16), Score(-39,  -1), Score(-10,  -7), Score(-46,  -5), Score(-59, -21),
  Score( -5,  -8), Score( -3,   7), Score(  2,   4), Score( 13, -17), Score( 22,  12), Score( 64,  -5), Score( 29,   1), Score(  4, -19),
  Score(-27,  -7), Score( -3,   3), Score(  7,  -6), Score( 41,   5), Score( 14,   7), Score( 14,   6), Score( -5, -11), Score(-24,  -3),
  Score( -3, -18), Score(-12, -16), Score(  2,   7), Score( 17,   2), Score( 24,  -7), Score( -3,  -6), Score(  2, -13), Score( -8, -22),
  Score(-19, -23), Score(  7, -19), Score(  0, -15), Score(  6,  -5), Score(  2,   2), Score(  5, -10), Score(  4, -14), Score(  9, -16),
  Score(  7, -33), Score(  0, -36), Score( 12, -34), Score(-13, -15), Score(  1, -16), Score( 16, -33), Score( 25, -26), Score( 16, -64),
  Score( -5, -46), Score( 20, -36), Score(-23, -27), Score(-23, -15), Score(-14, -26), Score(-14, -16), Score( -8, -63), Score(-23, -38)
};

inline std::array<Score, 64> king_pst {
  Score(-83, -116), Score(106, -11), Score(122,  13), Score(137, -34), Score(123, -25), Score( 72, 22), Score( 44,  18), Score(-34, -136),
  Score( -2,  -45), Score( 67,  40), Score( 37,  26), Score( 44,  16), Score( 43,  17), Score( 27, 36), Score(-13,  51), Score(-60,  -75),
  Score(-44,   18), Score( 47,  41), Score(  7,  40), Score( 17,  35), Score( -2,  25), Score( 12, 33), Score( 16,  52), Score(-47,   -5),
  Score(-16,   -8), Score( 52,  24), Score( 21,  34), Score(-12,  36), Score( -1,  31), Score(  5, 38), Score( 30,  23), Score(-69,   -4),
  Score(-23,  -25), Score( 25,   5), Score(  6,  17), Score(  7,  24), Score(-15,  32), Score(-15, 25), Score(-14,   7), Score(-63,  -20),
  Score(-49,  -31), Score( 16,  -5), Score(-13,   8), Score(-12,  15), Score(-24,  25), Score(-37, 21), Score(-10,   2), Score(-53,  -19),
  Score(-12,  -22), Score(-14,  -9), Score(  6,  -2), Score(-40,   5), Score(-36,  12), Score(-32, 13), Score( 13,  -5), Score(  0,  -27),
  Score(-52,  -44), Score( 16, -35), Score(  0, -16), Score(-73,   2), Score(-34, -20), Score(-50, -8), Score( 32, -38), Score(  7,  -66)
};

inline std::array<Score, 64> knight_pst {
  Score(-221,  -33), Score(-103, -12), Score(-59,   0), Score(-11, -25), Score( 38, -30), Score(-31, -18), Score(-103, -14), Score(-255, -54),
  Score( -66,  -31), Score( -40, -16), Score(  5, -15), Score( 23, -14), Score( 37,  -3), Score( 35, -40), Score( -11, -21), Score( -32, -22),
  Score( -22,  -37), Score(  -3, -17), Score( 42,  -3), Score( 39,   3), Score( 75, -12), Score(115, -13), Score(  10, -22), Score( -33, -28),
  Score( -23,  -32), Score(   6, -11), Score( 39,  -1), Score( 66,  15), Score( 19,  16), Score( 61,   5), Score(  11, -10), Score(  25, -26),
  Score( -25,  -28), Score( -13, -10), Score( 15,   3), Score( 19,  10), Score( 32,   9), Score( 11,  10), Score(  26, -11), Score(  -6, -27),
  Score( -39,  -54), Score( -27, -28), Score( -8, -19), Score(  4,  -5), Score( 20,  -5), Score(  1, -24), Score(  -4, -37), Score( -40, -29),
  Score( -46,  -60), Score( -54, -37), Score(-31, -40), Score( -8, -34), Score(-14, -37), Score(-20, -34), Score( -39, -29), Score( -13, -48),
  Score(-104, -159), Score( -44, -75), Score(-54, -46), Score(-53, -30), Score(-22, -46), Score(-24, -52), Score( -59, -67), Score(  -2, -37)
};

inline std::array<Score, 64> pawn_pst {
  Score(  0,  0), Score(  0,  0), Score(  0,  0), Score(  0,  0), Score(  0,  0), Score( 0,  0), Score( 0,  0), Score(  0,  0),
  Score(206,  0), Score(124, 24), Score( 96, 49), Score(113, 29), Score( 98, 34), Score(36, 54), Score( 9, 74), Score( 13, 26),
  Score( 19, 33), Score( 25, 39), Score( 43, 28), Score( 36, 15), Score( 58, 21), Score(77, 13), Score(64, 20), Score( 30,  6),
  Score( 10, 31), Score( 16, 25), Score(  9, 19), Score( 25,  1), Score( 23,  2), Score(12, -2), Score(-4,  9), Score( -7, 13),
  Score(  0, 18), Score(  4, 18), Score(  9, 12), Score( 10,  5), Score( 11,  4), Score( 6,  8), Score( 0,  5), Score(-11,  2),
  Score(  0, 13), Score( -1,  9), Score( -3, 10), Score(  8,  9), Score(  9, 12), Score( 6,  7), Score(18, -4), Score( -4, -4),
  Score( -7, 17), Score(-11,  9), Score(-14, 15), Score(-11, 12), Score(-14, 26), Score(23, 11), Score(29, -6), Score(-16, -2),
  Score(  0,  0), Score(  0,  0), Score(  0,  0), Score(  0,  0), Score(  0,  0), Score( 0,  0), Score( 0,  0), Score(  0,  0)
};

inline std::array<Score, 64> queen_pst {
  Score(-10,   5), Score(  4,  14), Score( 10,  10), Score( 13,  23), Score( 14,  20), Score( 86,   2), Score( 64,    9), Score( 57,    8),
  Score(-36,   5), Score(-60,  40), Score(-31,  40), Score(-10,  38), Score(-10,  49), Score( 43,  25), Score( 5,    25), Score( 39,   10),
  Score( -9, -22), Score(-17,   7), Score(-14,  38), Score( -4,  37), Score( 36,  40), Score( 63,  20), Score( 69,    2), Score( 16,   12),
  Score(-34, -10), Score(-28,  -5), Score(-30,  24), Score(-20,  45), Score(-16,  46), Score( -6,  32), Score(  7,   38), Score( -8,    5),
  Score(-22, -32), Score(-17,  -5), Score(-15,  -2), Score(-16,  23), Score(-10,  17), Score(-13,   4), Score(  4,   -3), Score(-13,    3),
  Score(-21, -50), Score( -4, -42), Score(-10, -20), Score(-13, -36), Score( -4, -18), Score(  7, -24), Score(  6,  -25), Score( -9,  -39),
  Score(-14, -73), Score( -3, -47), Score(  2, -64), Score(  4, -61), Score(  3, -57), Score( 26, -97), Score( 16, -108), Score(-12,  -64),
  Score(  8, -89), Score( -8, -85), Score( -5, -67), Score(  4, -61), Score(  5, -84), Score(-18, -96), Score(-58, -123), Score( -9, -110)
};

inline std::array<Score, 64> rook_pst {
  Score(  1,  46), Score(  3,  47), Score( 25,  33), Score( 31,  28), Score( 24,  36), Score( 24,  42), Score(35,  37), Score( 54,  29),
  Score(  3,  44), Score( -8,  41), Score( 28,  28), Score( 40,  23), Score( 39,  20), Score( 56,  22), Score(45,  20), Score( 39,  16),
  Score(-18,  49), Score(  8,  27), Score( 14,  28), Score( 31,  10), Score( 59,  12), Score( 67,  21), Score(51,  13), Score( 11,  32),
  Score(-41,  40), Score(-23,  35), Score(  4,  23), Score( 20,  14), Score( 16,  13), Score( 17,  21), Score(24,  20), Score(-11,  18),
  Score(-46,  28), Score(-43,  34), Score(-26,  23), Score(-16,  11), Score(-22,  10), Score(-23,  17), Score(-7,  11), Score(-45,  19),
  Score(-48,  16), Score(-35,  14), Score(-33,   1), Score(-31,  -6), Score(-10, -11), Score(-21,  -1), Score(-6,  -1), Score(-25,   3),
  Score(-45, -12), Score(-33,  -8), Score(-18, -12), Score( -4, -24), Score( -3, -23), Score(-10, -19), Score(-7, -21), Score(-68,  -6),
  Score(-16, -13), Score( -9, -15), Score(  0, -12), Score(  3, -20), Score(  7, -26), Score(  4, -16), Score( 8, -11), Score(-18, -19)
};
// clang-format on
