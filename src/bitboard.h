
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
constexpr uint64_t RANK1 = 0x00000000000000ff;
constexpr uint64_t RANK2 = 0x000000000000ff00;
constexpr uint64_t RANK3 = 0x0000000000ff0000;
constexpr uint64_t RANK4 = 0x00000000ff000000;
constexpr uint64_t RANK5 = 0x000000ff00000000;
constexpr uint64_t RANK6 = 0x0000ff0000000000;
constexpr uint64_t RANK7 = 0x00ff000000000000;
constexpr uint64_t RANK8 = 0xff00000000000000;

uint64_t bb_square[64];
uint64_t bb_rank[64];
uint64_t bb_file[64];
uint64_t bb_between[64][64];
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

inline const uint64_t &bbSquare(int sq) {
  return bb_square[sq];
}

inline const uint64_t &bbRank(int rank) {
  return bb_rank[rank];
}

inline const uint64_t &bbFile(int sq) {
  return bb_file[sq];
}

constexpr uint64_t northOne(const uint64_t &bb) {
  return bb << 8;
}

constexpr uint64_t southOne(const uint64_t &bb) {
  return bb >> 8;
}

constexpr uint64_t eastOne(const uint64_t &bb) {
  return (bb & ~HFILE_BB) << 1;
}

constexpr uint64_t westOne(const uint64_t &bb) {
  return (bb & ~AFILE_BB) >> 1;
}

constexpr uint64_t northEastOne(const uint64_t &bb) {
  return (bb & ~HFILE_BB) << 9;
}

constexpr uint64_t southEastOne(const uint64_t &bb) {
  return (bb & ~HFILE_BB) >> 7;
}

constexpr uint64_t southWestOne(const uint64_t &bb) {
  return (bb & ~AFILE_BB) >> 9;
}

inline uint64_t northWestOne(const uint64_t &bb) {
  return (bb & ~AFILE_BB) << 7;
}

inline uint64_t northFill(const uint64_t &bb) {
  uint64_t fill = bb;
  fill |= (fill << 8);
  fill |= (fill << 16);
  fill |= (fill << 32);
  return fill;
}

inline uint64_t southFill(const uint64_t &bb) {
  uint64_t fill = bb;
  fill |= (fill >> 8);
  fill |= (fill >> 16);
  fill |= (fill >> 32);
  return fill;
}

inline void initBetweenBitboards(const uint64_t from, uint64_t (*stepFunc)(const uint64_t &), int step) {
  uint64_t bb      = stepFunc(bbSquare(from));
  uint64_t to      = from + step;
  uint64_t between = 0;

  while (bb)
  {
    if (from < 64 && to < 64)
    {
      bb_between[from][to] = between;
      between |= bb;
      bb = stepFunc(bb);
      to += step;
    }
  }
}

void init() {
  for (const int sq : Squares)
  {
    bb_square[sq] = static_cast<uint64_t>(1) << sq;
    bb_rank[sq]   = RANK1 << (sq & 56);
    bb_file[sq]   = AFILE_BB << (sq & 7);
  }

  for (const int sq : Squares)
  {
    pawn_front_span[0][sq]        = northFill(northOne(bbSquare(sq)));
    pawn_front_span[1][sq]        = southFill(southOne(bbSquare(sq)));
    pawn_east_attack_span[0][sq]  = northFill(northEastOne(bbSquare(sq)));
    pawn_east_attack_span[1][sq]  = southFill(southEastOne(bbSquare(sq)));
    pawn_west_attack_span[0][sq]  = northFill(northWestOne(bbSquare(sq)));
    pawn_west_attack_span[1][sq]  = southFill(southWestOne(bbSquare(sq)));
    passed_pawn_front_span[0][sq] = pawn_east_attack_span[0][sq] | pawn_front_span[0][sq] | pawn_west_attack_span[0][sq];
    passed_pawn_front_span[1][sq] = pawn_east_attack_span[1][sq] | pawn_front_span[1][sq] | pawn_west_attack_span[1][sq];

    std::fill(std::begin(bb_between[sq]), std::end(bb_between[sq]), 0);

    initBetweenBitboards(sq, northOne, 8);
    initBetweenBitboards(sq, northEastOne, 9);
    initBetweenBitboards(sq, eastOne, 1);
    initBetweenBitboards(sq, southEastOne, -7);
    initBetweenBitboards(sq, southOne, -8);
    initBetweenBitboards(sq, southWestOne, -9);
    initBetweenBitboards(sq, westOne, -1);
    initBetweenBitboards(sq, northWestOne, 7);

    pawn_captures[sq] = (bbSquare(sq) & ~HFILE_BB) << 9;
    pawn_captures[sq] |= (bbSquare(sq) & ~AFILE_BB) << 7;
    pawn_captures[sq + 64] = (bbSquare(sq) & ~AFILE_BB) >> 9;
    pawn_captures[sq + 64] |= (bbSquare(sq) & ~HFILE_BB) >> 7;

    knight_attacks[sq] = (bbSquare(sq) & ~(AFILE_BB | BFILE_BB)) << 6;
    knight_attacks[sq] |= (bbSquare(sq) & ~AFILE_BB) << 15;
    knight_attacks[sq] |= (bbSquare(sq) & ~HFILE_BB) << 17;
    knight_attacks[sq] |= (bbSquare(sq) & ~(GFILE_BB | HFILE_BB)) << 10;
    knight_attacks[sq] |= (bbSquare(sq) & ~(GFILE_BB | HFILE_BB)) >> 6;
    knight_attacks[sq] |= (bbSquare(sq) & ~HFILE_BB) >> 15;
    knight_attacks[sq] |= (bbSquare(sq) & ~AFILE_BB) >> 17;
    knight_attacks[sq] |= (bbSquare(sq) & ~(AFILE_BB | BFILE_BB)) >> 10;

    king_attacks[sq] = (bbSquare(sq) & ~AFILE_BB) >> 1;
    king_attacks[sq] |= (bbSquare(sq) & ~AFILE_BB) << 7;
    king_attacks[sq] |= bbSquare(sq) << 8;
    king_attacks[sq] |= (bbSquare(sq) & ~HFILE_BB) << 9;
    king_attacks[sq] |= (bbSquare(sq) & ~HFILE_BB) << 1;
    king_attacks[sq] |= (bbSquare(sq) & ~HFILE_BB) >> 7;
    king_attacks[sq] |= bbSquare(sq) >> 8;
    king_attacks[sq] |= (bbSquare(sq) & ~AFILE_BB) >> 9;
  }
  corner_a1 = bbSquare(a1) | bbSquare(b1) | bbSquare(a2) | bbSquare(b2);
  corner_a8 = bbSquare(a8) | bbSquare(b8) | bbSquare(a7) | bbSquare(b7);
  corner_h1 = bbSquare(h1) | bbSquare(g1) | bbSquare(h2) | bbSquare(g2);
  corner_h8 = bbSquare(h8) | bbSquare(g8) | bbSquare(h7) | bbSquare(g7);
}

uint64_t (*pawnPush[2]) (const uint64_t &) = {northOne, southOne};
uint64_t (*pawnEastAttacks[2]) (const uint64_t &) = {northWestOne, southWestOne};
uint64_t (*pawnWestAttacks[2]) (const uint64_t &) = {northEastOne, southEastOne};
uint64_t (*pawnFill[2]) (const uint64_t &) = {northFill, southFill};

constexpr std::array<uint64_t, 2> rank_1{RANK1, RANK8};

constexpr std::array<uint64_t, 2> rank_3{RANK3, RANK6};

constexpr std::array<uint64_t, 2> rank_7{RANK7, RANK2};

constexpr std::array<uint64_t, 2> rank_6_and_7{RANK6 | RANK7, RANK2 | RANK3};

constexpr std::array<uint64_t, 2> rank_7_and_8{RANK7 | RANK8, RANK1 | RANK2};

constexpr std::array<int, 2> pawn_push_dist{8, -8};

constexpr std::array<int, 2> pawn_double_push_dist{16, -16};

constexpr std::array<int, 2> pawn_west_attack_dist{9, -7};

constexpr std::array<int, 2> pawn_east_attack_dist{7, -9};

constexpr void resetLSB(uint64_t &x) {
  x &= (x - 1);
}

constexpr uint8_t popCount(uint64_t x) {
  return std::popcount(x);
}

constexpr lsb(uint64_t x) {
  return std::countr_zero(x);
}

}// namespace bitboard

using namespace bitboard;