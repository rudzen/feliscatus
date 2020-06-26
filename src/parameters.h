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

inline std::array<int, 14> bishop_mob2_eg{-60, -27, -20, -10, -8, -2, 2, 3, 4, 1, 6, 10, 16, 3};
inline std::array<int, 14> bishop_mob2_mg{-27, -14, -6, -1, 3, 7, 5, 7, 11, 15, 10, 18, 12, 29};
inline std::array<int, 14> bishop_mob_eg {-35, -20, -29, -20, -13, -6, -4, 0, 5, 8, -1, 5, 7, 2};
inline std::array<int, 14> bishop_mob_mg {-23, -17, -13, -13, -6, 0, 5, 5, 10, 3, 2, 6, -14, 16};

inline int bishop_pair_eg     = 57;
inline int bishop_pair_mg     = 29;

inline int bishop_diagonal_eg = 0;
inline int bishop_diagonal_mg = 45;

inline int king_obstructs_rook  = -49;
inline std::array<int, 4> king_on_half_open{12, -9, -55, -97};
inline std::array<int, 4> king_on_open     {29, -13, -63, -221};
inline std::array<int, 4> king_pawn_shelter{-20, 1, 6, -7};

inline std::array<int, 9> knight_mob2_eg{-137, -56, -38, -17, -7, 0, 0, -5, -15};
inline std::array<int, 9> knight_mob2_mg{-59, -41, -21, -10, 1, 8, 14, 22, 24};
inline std::array<int, 9> knight_mob_eg {-40, 8, 14, 0, -5, -9, -10, -15, -8};
inline std::array<int, 9> knight_mob_mg {76, 23, 5, 3, 3, -2, -6, -14, -16};

inline std::array<int, 8> passed_pawn_eg             {0, -16, -7, 2, 12, 13, -115, 0};
inline std::array<int, 8> passed_pawn_king_dist_them {0, -66, -24, 5, 22, 33, 39, 21};
inline std::array<int, 8> passed_pawn_king_dist_us   {0, 23, 24, 0, -10, -11, 0, -17};
inline std::array<int, 8> passed_pawn_mg             {0, -16, -16, -12, 23, 68, 95, 0};
inline std::array<int, 8> passed_pawn_no_attacks     {0, 3, 3, 13, 24, 48, 85, 0};
inline std::array<int, 8> passed_pawn_no_them        {0, 2, 6, 22, 36, 64, 129, 0};
inline std::array<int, 8> passed_pawn_no_us          {0, 0, -1, 3, 17, 32, 121, 0};

inline std::array<int, 2> pawn_behind_eg           {4, -8};
inline std::array<int, 2> pawn_behind_mg           {-4, -28};
inline std::array<int, 2> pawn_doubled_eg          {-20, -15};
inline std::array<int, 2> pawn_doubled_mg          {-15, 1};
inline std::array<int, 2> pawn_isolated_eg         {-7, -32};
inline std::array<int, 2> pawn_isolated_mg         {-13, -28};

inline int queen_attack_king = 39;
inline std::array<int, 28> queen_mob_eg {-3, -30, -17, -27, -36, -37, -30, -24, -27, -21, -17, -11, -1, 4, 2, 5, 2, 6, 3, 1, -15, -16, -15, -34, -34, -28, -12, -16};
inline std::array<int, 28> queen_mob_mg {-19, -34, -15, -15, -13, -10, -10, -11, -6, -1, 2, -1, -3, 2, 3, 5, 7, 3, 9, 13, 25, 34, 33, 58, 44, 41, 14, 19};
inline int rook_attack_king  = 11;
inline std::array<int, 15> rook_mob_eg {-46, -28, -24, -9, -5, -2, 5, 9, 14, 19, 21, 27, 27, 15, 3};
inline std::array<int, 15> rook_mob_mg {-28, -20, -13, -16, -17, -10, -9, -3, -1, 2, 4, 1, 6, 20, 28};
inline int rook_open_file    = 13;

inline std::array<int, 64> bishop_pst_eg{7, 13,  12,  10,  24,  -5,  -19, -14, 2,   9,   3,   16,  -1,  -7,  -5,  -21, -8,  7,   4,   -17, 12,  -5,
                               1, -19, -7,  3,   -6,  5,   7,   6,   -11, -3,  -18, -16, 7,   2,   -7,  -6,  -13, -22, -23, -19, -15, -5,
                               2, -10, -14, -16, -33, -36, -34, -15, -16, -33, -26, -64, -46, -36, -27, -15, -26, -16, -63, -38};
inline std::array<int, 64> bishop_pst_mg{-65, -58, -27, -41, -46, -70, 15, -35, -40, -26, -43, -25, -39, -10, -46, -59, -5,  -3,  2,   13, 22, 64,
                               29,  4,   -27, -3,  7,   41,  14, 14,  -5,  -24, -3,  -12, 2,   17,  24,  -3,  2,   -8,  -19, 7,  0,  6,
                               2,   5,   4,   9,   7,   0,   12, -13, 1,   16,  25,  16,  -5,  20,  -23, -23, -14, -14, -8,  -23};
inline std::array<int, 64> king_pst_eg{-116, -11, 13, -34, -25, 22, 18, -136, -45, 40, 26, 16, 17, 36, 51, -75, 18,  41, 40, 35, 25, 33, 52, -5,  -8,  24,  34,  36, 31,  38, 23,  -4,
                             -25,  5,   17, 24,  32,  25, 7,  -20,  -31, -5, 8,  15, 25, 21, 2,  -19, -22, -9, -2, 5,  12, 13, -5, -27, -44, -35, -16, 2,  -20, -8, -38, -66};
inline std::array<int, 64> king_pst_mg{-83, 106, 122, 137, 123, 72,  44, -34, -2,  67,  37,  44, 43,  27, -13, -60, -44, 47,  7,   17, -2,  12,
                             16,  -47, -16, 52,  21,  -12, -1, 5,   30,  -69, -23, 25, 6,   7,  -15, -15, -14, -63, -49, 16, -13, -12,
                             -24, -37, -10, -53, -12, -14, 6,  -40, -36, -32, 13,  0,  -52, 16, 0,   -73, -34, -50, 32,  7};
inline std::array<int, 64> knight_pst_eg{-33, -12, 0,   -25, -30, -18, -14, -54, -31, -16, -15, -14, -3,   -40, -21, -22, -37, -17, -3,  3,   -12, -13,
                               -22, -28, -32, -11, -1,  15,  16,  5,   -10, -26, -28, -10, 3,    10,  9,   10,  -11, -27, -54, -28, -19, -5,
                               -5,  -24, -37, -29, -60, -37, -40, -34, -37, -34, -29, -48, -159, -75, -46, -30, -46, -52, -67, -37};
inline std::array<int, 64> knight_pst_mg{-221, -103, -59, -11, 38,  -31, -103, -255, -66, -40, 5,   23,  37,   35,  -11, -32, -22, -3,  42,  39,  75, 115,
                               10,   -33,  -23, 6,   39,  66,  19,   61,   11,  25,  -25, -13, 15,   19,  32,  11,  26,  -6,  -39, -27, -8, 4,
                               20,   1,    -4,  -40, -46, -54, -31,  -8,   -14, -20, -39, -13, -104, -44, -54, -53, -22, -24, -59, -2};
inline std::array<int, 64> pawn_pst_eg{0,  0,  0,  0, 0, 0, 0, 0, 0,  24, 49, 29, 34, 54, 74, 26, 33, 39, 28, 15, 21, 13, 20, 6,  31, 25, 19, 1, 2, -2, 9, 13,
                             18, 18, 12, 5, 4, 8, 5, 2, 13, 9,  10, 9,  12, 7,  -4, -4, 17, 9,  15, 12, 26, 11, -6, -2, 0,  0,  0,  0, 0, 0,  0, 0};

inline std::array<int, 64> pawn_pst_mg
       {  0,   0,   0,   0,   0,   0,  0,   0,
        206, 124,  96, 113,  98,  36,  9,  13,
         19,  25,  43,  36,  58,  77, 64,  30,
         10,  16,   9,  25,  23,  12, -4,  -7,
          0,   4,   9,  10,  11,   6,  0, -11,
          0,  -1,  -3,   8,   9,   6, 18,  -4,
         -7, -11, -14, -11, -14,  23, 29, -16,
          0,   0,   0,   0,   0,   0,  0,   0};

inline std::array<int, 64> queen_pst_eg
       {  5,  14,  10,  23,  20,   2,    9,    8,
          5,  40,  40,  38,  49,  25,   25,   10,
        -22,   7,  38,  37,  40,  20,    2,   12,
        -10,  -5,  24,  45,  46,  32,   38,    5,
        -32,  -5,  -2,  23,  17,   4,   -3,    3,
        -50, -42, -20, -36, -18, -24,  -25,  -39,
        -73, -47, -64, -61, -57, -97, -108,  -64,
        -89, -85, -67, -61, -84, -96, -123, -110};

inline std::array<int, 64> queen_pst_mg
       {-10,   4,  10,  13,  14,  86,  64,  57,
        -36, -60, -31, -10, -10,  43,   5,  39,
         -9, -17, -14,  -4,  36,  63,  69,  16,
        -34, -28, -30, -20, -16,  -6,   7,  -8,
        -22, -17, -15, -16, -10, -13,   4, -13,
        -21,  -4, -10, -13,  -4,   7,   6,  -9,
        -14,  -3,   2,   4,   3,  26,  16, -12,
          8,  -8,  -5,   4,   5, -18, -58,  -9};

inline std::array<int, 64> rook_pst_eg
       { 46,  47,  33,  28,  36,  42,  37, 29,
         44,  41,  28,  23,  20,  22,  20, 16,
         49,  27,  28,  10,  12,  21,  13, 32,
         40,  35,  23,  14,  13,  21,  20, 18,
         28,  34,  23,  11,  10,  17,  11, 19,
         16,  14,   1,  -6, -11,  -1,  -1,  3,
        -12,  -8, -12, -24, -23, -19, -21, -6,
        -13, -15, -12, -20, -26, -16, -11, -19};

inline std::array<int, 64> rook_pst_mg
       {  1,   3,  25,  31,  24,  24, 35,  54,
          3,  -8,  28,  40,  39,  56, 45,  39,
        -18,   8,  14,  31,  59,  67, 51,  11,
        -41, -23,   4,  20,  16,  17, 24, -11,
        -46, -43, -26, -16, -22, -23, -7, -45,
        -48, -35, -33, -31, -10, -21, -6, -25,
        -45, -33, -18,  -4,  -3, -10, -7, -68,
        -16,  -9,   0,   3,   7,   4,  8, -18};

// clang-format on
