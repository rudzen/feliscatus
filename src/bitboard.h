
#pragma once

#include <cstdint>
#include <bit>

namespace bitboard {

constexpr uint64_t AFILE = 0x0101010101010101;
constexpr uint64_t HFILE = 0x8080808080808080;
constexpr uint64_t BFILE = 0x0202020202020202;
constexpr uint64_t GFILE = 0x4040404040404040;
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

inline uint64_t northOne(const uint64_t &bb) {
  return bb << 8;
}

inline uint64_t southOne(const uint64_t &bb) {
  return bb >> 8;
}

inline uint64_t eastOne(const uint64_t &bb) {
  return (bb & ~HFILE) << 1;
}

inline uint64_t westOne(const uint64_t &bb) {
  return (bb & ~AFILE) >> 1;
}

inline uint64_t northEastOne(const uint64_t &bb) {
  return (bb & ~HFILE) << 9;
}

inline uint64_t southEastOne(const uint64_t &bb) {
  return (bb & ~HFILE) >> 7;
}

inline uint64_t southWestOne(const uint64_t &bb) {
  return (bb & ~AFILE) >> 9;
}

inline uint64_t northWestOne(const uint64_t &bb) {
  return (bb & ~AFILE) << 7;
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
  for (uint64_t sq = a1; sq <= h8; sq++)
  {
    bb_square[sq] = (uint64_t)1 << sq;
    bb_rank[sq]   = RANK1 << (sq & 56);
    bb_file[sq]   = AFILE << (sq & 7);
  }

  for (uint64_t sq = a1; sq <= h8; sq++)
  {
    pawn_front_span[0][sq]        = northFill(northOne(bbSquare(sq)));
    pawn_front_span[1][sq]        = southFill(southOne(bbSquare(sq)));
    pawn_east_attack_span[0][sq]  = northFill(northEastOne(bbSquare(sq)));
    pawn_east_attack_span[1][sq]  = southFill(southEastOne(bbSquare(sq)));
    pawn_west_attack_span[0][sq]  = northFill(northWestOne(bbSquare(sq)));
    pawn_west_attack_span[1][sq]  = southFill(southWestOne(bbSquare(sq)));
    passed_pawn_front_span[0][sq] = pawn_east_attack_span[0][sq] | pawn_front_span[0][sq] | pawn_west_attack_span[0][sq];
    passed_pawn_front_span[1][sq] = pawn_east_attack_span[1][sq] | pawn_front_span[1][sq] | pawn_west_attack_span[1][sq];

    for (uint64_t to = a1; to < h8; to++)
    {
      bb_between[sq][to] = 0;
    }
    initBetweenBitboards(sq, northOne, 8);
    initBetweenBitboards(sq, northEastOne, 9);
    initBetweenBitboards(sq, eastOne, 1);
    initBetweenBitboards(sq, southEastOne, -7);
    initBetweenBitboards(sq, southOne, -8);
    initBetweenBitboards(sq, southWestOne, -9);
    initBetweenBitboards(sq, westOne, -1);
    initBetweenBitboards(sq, northWestOne, 7);

    pawn_captures[sq] = (bbSquare(sq) & ~HFILE) << 9;
    pawn_captures[sq] |= (bbSquare(sq) & ~AFILE) << 7;
    pawn_captures[sq + 64] = (bbSquare(sq) & ~AFILE) >> 9;
    pawn_captures[sq + 64] |= (bbSquare(sq) & ~HFILE) >> 7;

    knight_attacks[sq] = (bbSquare(sq) & ~(AFILE | BFILE)) << 6;
    knight_attacks[sq] |= (bbSquare(sq) & ~AFILE) << 15;
    knight_attacks[sq] |= (bbSquare(sq) & ~HFILE) << 17;
    knight_attacks[sq] |= (bbSquare(sq) & ~(GFILE | HFILE)) << 10;
    knight_attacks[sq] |= (bbSquare(sq) & ~(GFILE | HFILE)) >> 6;
    knight_attacks[sq] |= (bbSquare(sq) & ~HFILE) >> 15;
    knight_attacks[sq] |= (bbSquare(sq) & ~AFILE) >> 17;
    knight_attacks[sq] |= (bbSquare(sq) & ~(AFILE | BFILE)) >> 10;

    king_attacks[sq] = (bbSquare(sq) & ~AFILE) >> 1;
    king_attacks[sq] |= (bbSquare(sq) & ~AFILE) << 7;
    king_attacks[sq] |= bbSquare(sq) << 8;
    king_attacks[sq] |= (bbSquare(sq) & ~HFILE) << 9;
    king_attacks[sq] |= (bbSquare(sq) & ~HFILE) << 1;
    king_attacks[sq] |= (bbSquare(sq) & ~HFILE) >> 7;
    king_attacks[sq] |= bbSquare(sq) >> 8;
    king_attacks[sq] |= (bbSquare(sq) & ~AFILE) >> 9;
  }
  corner_a1 = bbSquare(a1) | bbSquare(b1) | bbSquare(a2) | bbSquare(b2);
  corner_a8 = bbSquare(a8) | bbSquare(b8) | bbSquare(a7) | bbSquare(b7);
  corner_h1 = bbSquare(h1) | bbSquare(g1) | bbSquare(h2) | bbSquare(g2);
  corner_h8 = bbSquare(h8) | bbSquare(g8) | bbSquare(h7) | bbSquare(g7);
}

uint64_t (*pawnPush[2])

  (const uint64_t &) = {northOne, southOne};
uint64_t (*pawnEastAttacks[2])

  (const uint64_t &) = {northWestOne, southWestOne};
uint64_t (*pawnWestAttacks[2])

  (const uint64_t &) = {northEastOne, southEastOne};
uint64_t (*pawnFill[2])

  (const uint64_t &) = {northFill, southFill};

const uint64_t rank_1[2] = {RANK1, RANK8};

const uint64_t rank_3[2] = {RANK3, RANK6};

const uint64_t rank_7[2] = {RANK7, RANK2};

const uint64_t rank_6_and_7[2] = {RANK6 | RANK7, RANK2 | RANK3};

const uint64_t rank_7_and_8[2] = {RANK7 | RANK8, RANK1 | RANK2};

const int pawn_push_dist[2] = {8, -8};

const int pawn_double_push_dist[2] = {16, -16};

const int pawn_west_attack_dist[2] = {9, -7};

const int pawn_east_attack_dist[2] = {7, -9};

__forceinline void resetLSB(uint64_t &x) {
  x &= (x - 1);
}

#ifdef _M_X64
__forceinline uint8_t popCount(uint64_t x) {
  return std::popcount(x);
}

__forceinline int lsb(uint64_t x) {
  register unsigned long index;
  _BitScanForward64(&index, x);
  return index;
}
#else
__forceinline uint32_t popCount(uint64_t x) {
  uint32_t lo = (uint32_t)x;
  uint32_t hi = (uint32_t)(x >> 32);
  return _mm_popcnt_u32(lo) + _mm_popcnt_u32(hi);
}
__forceinline uint32_t lsb(uint64_t x) {
  uint32_t lo = (uint32_t)x;
  uint32_t hi = (uint32_t)(x >> 32);
  DWORD id;
  if (lo)
    _BitScanForward(&id, lo);
  else
  {
    _BitScanForward(&id, hi);
    id += 32;
  }
  return (uint32_t)id;
}
#endif

}// namespace bitboard

using namespace bitboard;