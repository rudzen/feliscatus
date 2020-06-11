
#pragma once

#include <cstdint>
#include <bit>
#include <algorithm>
#include "square.h"

namespace bitboard {

constexpr uint64_t AFILE_BB = 0x0101010101010101;
constexpr uint64_t HFILE_BB = 0x8080808080808080;
constexpr uint64_t BFILE_BB = 0x0202020202020202;
constexpr uint64_t GFILE_BB = 0x4040404040404040;
constexpr uint64_t RANK1    = 0x00000000000000ff;
constexpr uint64_t RANK2    = 0x000000000000ff00;
constexpr uint64_t RANK3    = 0x0000000000ff0000;
constexpr uint64_t RANK4    = 0x00000000ff000000;
constexpr uint64_t RANK5    = 0x000000ff00000000;
constexpr uint64_t RANK6    = 0x0000ff0000000000;
constexpr uint64_t RANK7    = 0x00ff000000000000;
constexpr uint64_t RANK8    = 0xff00000000000000;

uint64_t square_bb[64];
uint64_t rank_bb[64];
uint64_t file_bb[64];
uint64_t between_bb[64][64];
uint64_t passed_pawn_front_span[2][64];
uint64_t pawn_front_span[2][64];
uint64_t pawn_east_attack_span[2][64];
uint64_t pawn_west_attack_span[2][64];
uint64_t pawn_captures[128];
uint64_t knight_attacks[64];
uint64_t king_attacks[64];
uint64_t corner_a1;
uint64_t corner_a8;
uint64_t corner_h1;
uint64_t corner_h8;

inline const uint64_t &bb_square(const int sq) {
  return square_bb[sq];
}

inline const uint64_t &bb_rank(const int rank) {
  return rank_bb[rank];
}

inline const uint64_t &bb_file(const int sq) {
  return file_bb[sq];
}

constexpr uint64_t north_one(const uint64_t &bb) {
  return bb << 8;
}

constexpr uint64_t south_one(const uint64_t &bb) {
  return bb >> 8;
}

constexpr uint64_t east_one(const uint64_t &bb) {
  return (bb & ~HFILE_BB) << 1;
}

constexpr uint64_t west_one(const uint64_t &bb) {
  return (bb & ~AFILE_BB) >> 1;
}

constexpr uint64_t north_east_one(const uint64_t &bb) {
  return (bb & ~HFILE_BB) << 9;
}

constexpr uint64_t south_east_one(const uint64_t &bb) {
  return (bb & ~HFILE_BB) >> 7;
}

constexpr uint64_t south_west_one(const uint64_t &bb) {
  return (bb & ~AFILE_BB) >> 9;
}

inline uint64_t north_west_one(const uint64_t &bb) {
  return (bb & ~AFILE_BB) << 7;
}

inline uint64_t north_fill(const uint64_t &bb) {
  auto fill = bb;
  fill |= (fill << 8);
  fill |= (fill << 16);
  fill |= (fill << 32);
  return fill;
}

inline uint64_t south_fill(const uint64_t &bb) {
  auto fill = bb;
  fill |= (fill >> 8);
  fill |= (fill >> 16);
  fill |= (fill >> 32);
  return fill;
}

inline void init_between_bitboards(const uint64_t from, uint64_t (*step_func)(const uint64_t &), int step) {
  auto bb          = step_func(bb_square(from));
  auto to          = from + step;
  uint64_t between = 0;

  while (bb)
  {
    if (from < 64 && to < 64)
    {
      between_bb[from][to] = between;
      between |= bb;
      bb = step_func(bb);
      to += step;
    }
  }
}

void init() {
  for (const auto sq : Squares)
  {
    square_bb[sq] = static_cast<uint64_t>(1) << sq;
    rank_bb[sq]   = RANK1 << (sq & 56);
    file_bb[sq]   = AFILE_BB << (sq & 7);
  }

  for (const auto sq : Squares)
  {
    pawn_front_span[0][sq]        = north_fill(north_one(bb_square(sq)));
    pawn_front_span[1][sq]        = south_fill(south_one(bb_square(sq)));
    pawn_east_attack_span[0][sq]  = north_fill(north_east_one(bb_square(sq)));
    pawn_east_attack_span[1][sq]  = south_fill(south_east_one(bb_square(sq)));
    pawn_west_attack_span[0][sq]  = north_fill(north_west_one(bb_square(sq)));
    pawn_west_attack_span[1][sq]  = south_fill(south_west_one(bb_square(sq)));
    passed_pawn_front_span[0][sq] = pawn_east_attack_span[0][sq] | pawn_front_span[0][sq] | pawn_west_attack_span[0][sq];
    passed_pawn_front_span[1][sq] = pawn_east_attack_span[1][sq] | pawn_front_span[1][sq] | pawn_west_attack_span[1][sq];

    std::fill(std::begin(between_bb[sq]), std::end(between_bb[sq]), 0);

    init_between_bitboards(sq, north_one, 8);
    init_between_bitboards(sq, north_east_one, 9);
    init_between_bitboards(sq, east_one, 1);
    init_between_bitboards(sq, south_east_one, -7);
    init_between_bitboards(sq, south_one, -8);
    init_between_bitboards(sq, south_west_one, -9);
    init_between_bitboards(sq, west_one, -1);
    init_between_bitboards(sq, north_west_one, 7);

    pawn_captures[sq] = (bb_square(sq) & ~HFILE_BB) << 9;
    pawn_captures[sq] |= (bb_square(sq) & ~AFILE_BB) << 7;
    pawn_captures[sq + 64] = (bb_square(sq) & ~AFILE_BB) >> 9;
    pawn_captures[sq + 64] |= (bb_square(sq) & ~HFILE_BB) >> 7;

    knight_attacks[sq] = (bb_square(sq) & ~(AFILE_BB | BFILE_BB)) << 6;
    knight_attacks[sq] |= (bb_square(sq) & ~AFILE_BB) << 15;
    knight_attacks[sq] |= (bb_square(sq) & ~HFILE_BB) << 17;
    knight_attacks[sq] |= (bb_square(sq) & ~(GFILE_BB | HFILE_BB)) << 10;
    knight_attacks[sq] |= (bb_square(sq) & ~(GFILE_BB | HFILE_BB)) >> 6;
    knight_attacks[sq] |= (bb_square(sq) & ~HFILE_BB) >> 15;
    knight_attacks[sq] |= (bb_square(sq) & ~AFILE_BB) >> 17;
    knight_attacks[sq] |= (bb_square(sq) & ~(AFILE_BB | BFILE_BB)) >> 10;

    king_attacks[sq] = (bb_square(sq) & ~AFILE_BB) >> 1;
    king_attacks[sq] |= (bb_square(sq) & ~AFILE_BB) << 7;
    king_attacks[sq] |= bb_square(sq) << 8;
    king_attacks[sq] |= (bb_square(sq) & ~HFILE_BB) << 9;
    king_attacks[sq] |= (bb_square(sq) & ~HFILE_BB) << 1;
    king_attacks[sq] |= (bb_square(sq) & ~HFILE_BB) >> 7;
    king_attacks[sq] |= bb_square(sq) >> 8;
    king_attacks[sq] |= (bb_square(sq) & ~AFILE_BB) >> 9;
  }
  corner_a1 = bb_square(a1) | bb_square(b1) | bb_square(a2) | bb_square(b2);
  corner_a8 = bb_square(a8) | bb_square(b8) | bb_square(a7) | bb_square(b7);
  corner_h1 = bb_square(h1) | bb_square(g1) | bb_square(h2) | bb_square(g2);
  corner_h8 = bb_square(h8) | bb_square(g8) | bb_square(h7) | bb_square(g7);
}

inline uint64_t (*pawn_push[2])(const uint64_t &)         = {north_one, south_one};
inline uint64_t (*pawn_east_attacks[2])(const uint64_t &) = {north_west_one, south_west_one};
inline uint64_t (*pawn_west_attacks[2])(const uint64_t &) = {north_east_one, south_east_one};
inline uint64_t (*pawn_fill[2])(const uint64_t &)         = {north_fill, south_fill};

constexpr std::array<uint64_t, 2> rank_1{RANK1, RANK8};

constexpr std::array<uint64_t, 2> rank_3{RANK3, RANK6};

constexpr std::array<uint64_t, 2> rank_7{RANK7, RANK2};

constexpr std::array<uint64_t, 2> rank_6_and_7{RANK6 | RANK7, RANK2 | RANK3};

constexpr std::array<uint64_t, 2> rank_7_and_8{RANK7 | RANK8, RANK1 | RANK2};

constexpr std::array<int, 2> pawn_push_dist{8, -8};

constexpr std::array<int, 2> pawn_double_push_dist{16, -16};

constexpr std::array<int, 2> pawn_west_attack_dist{9, -7};

constexpr std::array<int, 2> pawn_east_attack_dist{7, -9};

constexpr void reset_lsb(uint64_t &x) {
  x &= (x - 1);
}

constexpr uint8_t pop_count(const uint64_t x) {
  return std::popcount(x);
}

constexpr int lsb(const uint64_t x) {
  return std::countr_zero(x);
}

}// namespace bitboard

using namespace bitboard;