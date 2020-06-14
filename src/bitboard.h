
#pragma once

#include <cstdint>
#include <bit>
#include <algorithm>
#include "square.h"
#include "types.h"

namespace bitboard {

constexpr static Bitboard AllSquares  = ~Bitboard(0);
constexpr static Bitboard DarkSquares = 0xAA55AA55AA55AA55ULL;
constexpr static Bitboard ZeroBB      = ~AllSquares;
constexpr static Bitboard OneBB       = 0x1ULL;

//------------------------------------------------
// constexpr generate functions
//------------------------------------------------
template<typename... Squares>
constexpr Bitboard make_bitboard(Squares... squares) {
  return (... | (OneBB << squares));
}

constexpr Bitboard FileABB = 0x0101010101010101ULL;
constexpr Bitboard FileBBB = FileABB << 1;
constexpr Bitboard FileCBB = FileABB << 2;
constexpr Bitboard FileDBB = FileABB << 3;
constexpr Bitboard FileEBB = FileABB << 4;
constexpr Bitboard FileFBB = FileABB << 5;
constexpr Bitboard FileGBB = FileABB << 6;
constexpr Bitboard FileHBB = FileABB << 7;

constexpr Bitboard Rank1BB = 0xFF;
constexpr Bitboard Rank2BB = Rank1BB << (8 * 1);
constexpr Bitboard Rank3BB = Rank1BB << (8 * 2);
constexpr Bitboard Rank4BB = Rank1BB << (8 * 3);
constexpr Bitboard Rank5BB = Rank1BB << (8 * 4);
constexpr Bitboard Rank6BB = Rank1BB << (8 * 5);
constexpr Bitboard Rank7BB = Rank1BB << (8 * 6);
constexpr Bitboard Rank8BB = Rank1BB << (8 * 7);

constexpr Bitboard corner_a1 = make_bitboard(a1, b1, a2, b2);
constexpr Bitboard corner_a8 = make_bitboard(a8, b8, a7, b7);
constexpr Bitboard corner_h1 = make_bitboard(h1, g1, h2, g2);
constexpr Bitboard corner_h8 = make_bitboard(h8, g8, h7, g7);

constexpr std::array<Bitboard, sq_nb> square_bb{
  make_bitboard(a1), make_bitboard(b1), make_bitboard(c1), make_bitboard(d1), make_bitboard(e1), make_bitboard(f1), make_bitboard(g1), make_bitboard(h1),
  make_bitboard(a2), make_bitboard(b2), make_bitboard(c2), make_bitboard(d2), make_bitboard(e2), make_bitboard(f2), make_bitboard(g2), make_bitboard(h2),
  make_bitboard(a3), make_bitboard(b3), make_bitboard(c3), make_bitboard(d3), make_bitboard(e3), make_bitboard(f3), make_bitboard(g3), make_bitboard(h3),
  make_bitboard(a4), make_bitboard(b4), make_bitboard(c4), make_bitboard(d4), make_bitboard(e4), make_bitboard(f4), make_bitboard(g4), make_bitboard(h4),
  make_bitboard(a5), make_bitboard(b5), make_bitboard(c5), make_bitboard(d5), make_bitboard(e5), make_bitboard(f5), make_bitboard(g5), make_bitboard(h5),
  make_bitboard(a6), make_bitboard(b6), make_bitboard(c6), make_bitboard(d6), make_bitboard(e6), make_bitboard(f6), make_bitboard(g6), make_bitboard(h6),
  make_bitboard(a7), make_bitboard(b7), make_bitboard(c7), make_bitboard(d7), make_bitboard(e7), make_bitboard(f7), make_bitboard(g7), make_bitboard(h7),
  make_bitboard(a8), make_bitboard(b8), make_bitboard(c8), make_bitboard(d8), make_bitboard(e8), make_bitboard(f8), make_bitboard(g8), make_bitboard(h8)};

constexpr std::array<Bitboard, 8> RankBB{Rank1BB, Rank2BB, Rank3BB, Rank4BB, Rank5BB, Rank6BB, Rank7BB, Rank8BB};
constexpr std::array<Bitboard, 8> FileBB{FileABB, FileBBB, FileCBB, FileDBB, FileEBB, FileFBB, FileGBB, FileHBB};

constexpr std::array<Bitboard, sq_nb> make_knight_attacks()
{
  std::array<Bitboard, sq_nb> result{};
  for (const Square sq : Squares)
  {
    const auto bbsq = square_bb[sq];
    result[sq] =  (bbsq & ~(FileABB | FileBBB)) << 6;
    result[sq] |= (bbsq & ~FileABB) << 15;
    result[sq] |= (bbsq & ~FileHBB) << 17;
    result[sq] |= (bbsq & ~(FileGBB | FileHBB)) << 10;
    result[sq] |= (bbsq & ~(FileGBB | FileHBB)) >> 6;
    result[sq] |= (bbsq & ~FileHBB) >> 15;
    result[sq] |= (bbsq & ~FileABB) >> 17;
    result[sq] |= (bbsq & ~(FileABB | FileBBB)) >> 10;
  }

  return result;
}

constexpr std::array<Bitboard, sq_nb> knight_attacks = make_knight_attacks();

constexpr std::array<Bitboard, sq_nb> make_king_attacks()
{
  std::array<Bitboard, sq_nb> result{};
  for (const Square sq : Squares)
  {
    const auto bbsq = square_bb[sq];
    result[sq] =  (bbsq & ~FileABB) >> 1;
    result[sq] |= (bbsq & ~FileABB) << 7;
    result[sq] |= bbsq << 8;
    result[sq] |= (bbsq & ~FileHBB) << 9;
    result[sq] |= (bbsq & ~FileHBB) << 1;
    result[sq] |= (bbsq & ~FileHBB) >> 7;
    result[sq] |= bbsq >> 8;
    result[sq] |= (bbsq & ~FileABB) >> 9;
  }

  return result;
}

constexpr std::array<Bitboard, sq_nb> king_attacks = make_king_attacks();

inline Bitboard between_bb[64][64];
inline Bitboard passed_pawn_front_span[2][64];
inline Bitboard pawn_front_span[2][64];
inline Bitboard pawn_east_attack_span[2][64];
inline Bitboard pawn_west_attack_span[2][64];
inline Bitboard pawn_captures[128];

constexpr Bitboard bb_square(const Square sq) {
  return square_bb[sq];
}

constexpr Bitboard bb_rank(const Rank rank) {
  return RankBB[rank];
}

constexpr Bitboard bb_file(const File file) {
  return FileBB[file];
}

constexpr Bitboard bb_file(const Square s) {
  return bb_file(file_of(s));
}

constexpr Bitboard north_one(const Bitboard bb) {
  return bb << 8;
}

constexpr Bitboard south_one(const Bitboard bb) {
  return bb >> 8;
}

constexpr Bitboard east_one(const Bitboard bb) {
  return (bb & ~FileHBB) << 1;
}

constexpr Bitboard west_one(const Bitboard bb) {
  return (bb & ~FileABB) >> 1;
}

constexpr Bitboard north_east_one(const Bitboard bb) {
  return (bb & ~FileHBB) << 9;
}

constexpr Bitboard south_east_one(const Bitboard bb) {
  return (bb & ~FileHBB) >> 7;
}

constexpr Bitboard south_west_one(const Bitboard bb) {
  return (bb & ~FileABB) >> 9;
}

constexpr Bitboard north_west_one(const Bitboard bb) {
  return (bb & ~FileABB) << 7;
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

constexpr void init_between_bitboards(const Square from, Bitboard (*step_func)(Bitboard), const int step) {
  auto bb          = step_func(bb_square(from));
  auto to          = from + step;
  Bitboard between = ZeroBB;

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

constexpr void init() {

  for (const auto sq : Squares)
  {
    const auto bbsq = square_bb[sq];

    pawn_front_span[WHITE][sq]        = north_fill(north_one(bbsq));
    pawn_front_span[BLACK][sq]        = south_fill(south_one(bbsq));
    pawn_east_attack_span[WHITE][sq]  = north_fill(north_east_one(bbsq));
    pawn_east_attack_span[BLACK][sq]  = south_fill(south_east_one(bbsq));
    pawn_west_attack_span[WHITE][sq]  = north_fill(north_west_one(bbsq));
    pawn_west_attack_span[BLACK][sq]  = south_fill(south_west_one(bbsq));
    passed_pawn_front_span[WHITE][sq] = pawn_east_attack_span[WHITE][sq] | pawn_front_span[WHITE][sq] | pawn_west_attack_span[WHITE][sq];
    passed_pawn_front_span[BLACK][sq] = pawn_east_attack_span[BLACK][sq] | pawn_front_span[BLACK][sq] | pawn_west_attack_span[BLACK][sq];

    std::fill(std::begin(between_bb[sq]), std::end(between_bb[sq]), 0);

    init_between_bitboards(sq, north_one, 8);
    init_between_bitboards(sq, north_east_one, 9);
    init_between_bitboards(sq, east_one, 1);
    init_between_bitboards(sq, south_east_one, -7);
    init_between_bitboards(sq, south_one, -8);
    init_between_bitboards(sq, south_west_one, -9);
    init_between_bitboards(sq, west_one, -1);
    init_between_bitboards(sq, north_west_one, 7);

    pawn_captures[sq] = (bbsq & ~FileHBB) << 9;
    pawn_captures[sq] |= (bbsq & ~FileABB) << 7;
    pawn_captures[sq + 64] = (bbsq & ~FileABB) >> 9;
    pawn_captures[sq + 64] |= (bbsq & ~FileHBB) >> 7;
  }
}

constexpr Bitboard (*pawn_push[2])(Bitboard)         = {north_one, south_one};
constexpr Bitboard (*pawn_east_attacks[2])(Bitboard) = {north_west_one, south_west_one};
constexpr Bitboard (*pawn_west_attacks[2])(Bitboard) = {north_east_one, south_east_one};
constexpr Bitboard (*pawn_fill[2])(Bitboard)         = {north_fill, south_fill};

constexpr std::array<Bitboard, 2> rank_1{Rank1BB, Rank8BB};

constexpr std::array<Bitboard, 2> rank_3{Rank3BB, Rank6BB};

constexpr std::array<Bitboard, 2> rank_7{Rank7BB, Rank2BB};

constexpr std::array<Bitboard, 2> rank_6_and_7{Rank6BB | Rank7BB, Rank2BB | Rank3BB};

constexpr std::array<Bitboard, 2> rank_7_and_8{Rank7BB | Rank8BB, Rank1BB | Rank2BB};

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