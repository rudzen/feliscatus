
#pragma once

#include <cstdint>
#include <bit>
#include <algorithm>
#include "square.h"
#include "types.h"

namespace bitboard {

constexpr Bitboard AFILE_BB = 0x0101010101010101;
constexpr Bitboard HFILE_BB = 0x8080808080808080;
constexpr Bitboard BFILE_BB = 0x0202020202020202;
constexpr Bitboard GFILE_BB = 0x4040404040404040;
constexpr Bitboard RANK1    = 0x00000000000000ff;
constexpr Bitboard RANK2    = 0x000000000000ff00;
constexpr Bitboard RANK3    = 0x0000000000ff0000;
constexpr Bitboard RANK4    = 0x00000000ff000000;
constexpr Bitboard RANK5    = 0x000000ff00000000;
constexpr Bitboard RANK6    = 0x0000ff0000000000;
constexpr Bitboard RANK7    = 0x00ff000000000000;
constexpr Bitboard RANK8    = 0xff00000000000000;

inline Bitboard square_bb[64];
inline Bitboard rank_bb[64];
inline Bitboard file_bb[64];
inline Bitboard between_bb[64][64];
inline Bitboard passed_pawn_front_span[2][64];
inline Bitboard pawn_front_span[2][64];
inline Bitboard pawn_east_attack_span[2][64];
inline Bitboard pawn_west_attack_span[2][64];
inline Bitboard pawn_captures[128];
inline Bitboard knight_attacks[64];
inline Bitboard king_attacks[64];
inline Bitboard corner_a1;
inline Bitboard corner_a8;
inline Bitboard corner_h1;
inline Bitboard corner_h8;

inline Bitboard bb_square(const Square sq) {
  return square_bb[sq];
}

inline Bitboard bb_rank(const int rank) {
  return rank_bb[rank];
}

inline Bitboard bb_file(const Square sq) {
  return file_bb[sq];
}

constexpr Bitboard north_one(const Bitboard bb) {
  return bb << 8;
}

constexpr Bitboard south_one(const Bitboard bb) {
  return bb >> 8;
}

constexpr Bitboard east_one(const Bitboard bb) {
  return (bb & ~HFILE_BB) << 1;
}

constexpr Bitboard west_one(const Bitboard bb) {
  return (bb & ~AFILE_BB) >> 1;
}

constexpr Bitboard north_east_one(const Bitboard bb) {
  return (bb & ~HFILE_BB) << 9;
}

constexpr Bitboard south_east_one(const Bitboard bb) {
  return (bb & ~HFILE_BB) >> 7;
}

constexpr Bitboard south_west_one(const Bitboard bb) {
  return (bb & ~AFILE_BB) >> 9;
}

constexpr Bitboard north_west_one(const Bitboard bb) {
  return (bb & ~AFILE_BB) << 7;
}

constexpr Bitboard north_fill(const Bitboard bb) {
  auto fill = bb;
  fill |= (fill << 8);
  fill |= (fill << 16);
  fill |= (fill << 32);
  return fill;
}

constexpr Bitboard south_fill(const Bitboard bb) {
  auto fill = bb;
  fill |= (fill >> 8);
  fill |= (fill >> 16);
  fill |= (fill >> 32);
  return fill;
}

inline void init_between_bitboards(const Square from, Bitboard (*step_func)(Bitboard), const int step) {
  auto bb          = step_func(bb_square(from));
  auto to          = from + step;
  Bitboard between = 0;

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

inline void init() {
  for (const auto sq : Squares)
  {
    square_bb[sq] = static_cast<uint64_t>(1) << sq;
    rank_bb[sq]   = RANK1 << (sq & 56);
    file_bb[sq]   = AFILE_BB << (sq & 7);
  }

  for (const auto sq : Squares)
  {
    const auto bbsq = bb_square(sq);

    pawn_front_span[0][sq]        = north_fill(north_one(bbsq));
    pawn_front_span[1][sq]        = south_fill(south_one(bbsq));
    pawn_east_attack_span[0][sq]  = north_fill(north_east_one(bbsq));
    pawn_east_attack_span[1][sq]  = south_fill(south_east_one(bbsq));
    pawn_west_attack_span[0][sq]  = north_fill(north_west_one(bbsq));
    pawn_west_attack_span[1][sq]  = south_fill(south_west_one(bbsq));
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

    pawn_captures[sq] = (bbsq & ~HFILE_BB) << 9;
    pawn_captures[sq] |= (bbsq & ~AFILE_BB) << 7;
    pawn_captures[sq + 64] = (bbsq & ~AFILE_BB) >> 9;
    pawn_captures[sq + 64] |= (bbsq & ~HFILE_BB) >> 7;

    knight_attacks[sq] = (bbsq & ~(AFILE_BB | BFILE_BB)) << 6;
    knight_attacks[sq] |= (bbsq & ~AFILE_BB) << 15;
    knight_attacks[sq] |= (bbsq & ~HFILE_BB) << 17;
    knight_attacks[sq] |= (bbsq & ~(GFILE_BB | HFILE_BB)) << 10;
    knight_attacks[sq] |= (bbsq & ~(GFILE_BB | HFILE_BB)) >> 6;
    knight_attacks[sq] |= (bbsq & ~HFILE_BB) >> 15;
    knight_attacks[sq] |= (bbsq & ~AFILE_BB) >> 17;
    knight_attacks[sq] |= (bbsq & ~(AFILE_BB | BFILE_BB)) >> 10;

    king_attacks[sq] = (bbsq & ~AFILE_BB) >> 1;
    king_attacks[sq] |= (bbsq & ~AFILE_BB) << 7;
    king_attacks[sq] |= bbsq << 8;
    king_attacks[sq] |= (bbsq & ~HFILE_BB) << 9;
    king_attacks[sq] |= (bbsq & ~HFILE_BB) << 1;
    king_attacks[sq] |= (bbsq & ~HFILE_BB) >> 7;
    king_attacks[sq] |= bbsq >> 8;
    king_attacks[sq] |= (bbsq & ~AFILE_BB) >> 9;
  }
  corner_a1 = bb_square(a1) | bb_square(b1) | bb_square(a2) | bb_square(b2);
  corner_a8 = bb_square(a8) | bb_square(b8) | bb_square(a7) | bb_square(b7);
  corner_h1 = bb_square(h1) | bb_square(g1) | bb_square(h2) | bb_square(g2);
  corner_h8 = bb_square(h8) | bb_square(g8) | bb_square(h7) | bb_square(g7);
}

constexpr Bitboard (*pawn_push[2])(Bitboard)         = {north_one, south_one};
constexpr Bitboard (*pawn_east_attacks[2])(Bitboard) = {north_west_one, south_west_one};
constexpr Bitboard (*pawn_west_attacks[2])(Bitboard) = {north_east_one, south_east_one};
constexpr Bitboard (*pawn_fill[2])(Bitboard)         = {north_fill, south_fill};

constexpr std::array<Bitboard, 2> rank_1{RANK1, RANK8};

constexpr std::array<Bitboard, 2> rank_3{RANK3, RANK6};

constexpr std::array<Bitboard, 2> rank_7{RANK7, RANK2};

constexpr std::array<Bitboard, 2> rank_6_and_7{RANK6 | RANK7, RANK2 | RANK3};

constexpr std::array<Bitboard, 2> rank_7_and_8{RANK7 | RANK8, RANK1 | RANK2};

constexpr std::array<int, 2> pawn_push_dist{8, -8};

constexpr std::array<int, 2> pawn_double_push_dist{16, -16};

constexpr std::array<int, 2> pawn_west_attack_dist{9, -7};

constexpr std::array<int, 2> pawn_east_attack_dist{7, -9};

constexpr void reset_lsb(Bitboard &x) {
  x &= (x - 1);
}

constexpr uint8_t pop_count(const Bitboard x) {
  return std::popcount(x);
}

constexpr Square lsb(const Bitboard x) {
  return static_cast<Square>(std::countr_zero(x));
}

}// namespace bitboard

using namespace bitboard;