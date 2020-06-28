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

#include <cstdint>
#include <bit>
#include <algorithm>
#include <string_view>

#include "types.h"
#include "util.h"

namespace bitboard {

std::string print_bitboard(Bitboard b, std::string_view title);

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

constexpr std::array<Bitboard, RANK_NB> RankBB{Rank1BB, Rank2BB, Rank3BB, Rank4BB, Rank5BB, Rank6BB, Rank7BB, Rank8BB};
constexpr std::array<Bitboard, FILE_NB> FileBB{FileABB, FileBBB, FileCBB, FileDBB, FileEBB, FileFBB, FileGBB, FileHBB};
constexpr std::array<Bitboard, COL_NB> rank_1{Rank1BB, Rank8BB};
constexpr std::array<Bitboard, COL_NB> rank_3{Rank3BB, Rank6BB};
constexpr std::array<Bitboard, COL_NB> rank_7{Rank7BB, Rank2BB};
constexpr std::array<Bitboard, COL_NB> rank_6_and_7{Rank6BB | Rank7BB, Rank2BB | Rank3BB};
constexpr std::array<Bitboard, COL_NB> rank_7_and_8{Rank7BB | Rank8BB, Rank1BB | Rank2BB};
constexpr std::array<Direction, COL_NB> pawn_push_dist{NORTH, SOUTH};
constexpr std::array<Direction, COL_NB> pawn_double_push_dist{NORTH * 2, SOUTH * 2};
constexpr std::array<Direction, COL_NB> pawn_west_attack_dist{NORTH_EAST, SOUTH_EAST};
constexpr std::array<Direction, COL_NB> pawn_east_attack_dist{NORTH_WEST, SOUTH_WEST};

consteval std::array<Bitboard, sq_nb> make_knight_attacks()
{
  std::array<Bitboard, sq_nb> result{};
  for (const auto sq : Squares)
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

consteval std::array<Bitboard, sq_nb> make_king_attacks()
{
  std::array<Bitboard, sq_nb> result{};
  for (const auto sq : Squares)
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

template<typename T1 = Square>
constexpr int distance(const Square x, const Square y);
template<>
constexpr int distance<File>(const Square x, const Square y) { return util::abs(file_of(x) - file_of(y)); }
template<>
constexpr int distance<Rank>(const Square x, const Square y) { return util::abs(rank_of(x) - rank_of(y)); }

inline Bitboard between_bb[sq_nb][sq_nb];
inline Bitboard passed_pawn_front_span[COL_NB][sq_nb];
inline Bitboard pawn_front_span[COL_NB][sq_nb];
inline Bitboard pawn_captures[COL_NB][sq_nb];

consteval std::array<std::array<int, sq_nb>, sq_nb> make_distance()
{
  std::array<std::array<int, sq_nb>, sq_nb> result{};
  for (const auto sq1 : Squares)
    for (const auto sq2 : Squares)
      result[sq1][sq2] = std::max(distance<Rank>(sq1, sq2), distance<File>(sq1, sq2));

  return result;
}

constexpr std::array<std::array<int, sq_nb>, sq_nb> dist = make_distance(); /// chebyshev distance

template<>
constexpr int distance<Square>(const Square x, const Square y) { return dist[x][y]; }

template<Square Sq>
constexpr Bitboard bit() {
  return square_bb[Sq];
}

template<typename... Squares>
constexpr Bitboard bit(Squares... squares) {
  return (... | square_bb[squares]);
}

constexpr Bitboard bit() {
  return 0;
}

template<typename... Ranks>
constexpr Bitboard bit(const Rank r, Ranks... ra) {
  return RankBB[r] | bit(ra...);
}

template<typename... Files>
constexpr Bitboard bit(const File f, Files... fi) {
  return FileBB[f] | bit(fi...);
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

//------------------------------------------------
// Additional Square operators
//------------------------------------------------
constexpr Bitboard operator&(const Bitboard b, const Square s) noexcept {
  return b & square_bb[s];
}
constexpr Bitboard operator|(const Bitboard b, const Square s) noexcept {
  return b | square_bb[s];
}
constexpr Bitboard operator^(const Bitboard b, const Square s) noexcept {
  return b ^ square_bb[s];
}
constexpr Bitboard &operator|=(Bitboard &b, const Square s) noexcept {
  return b |= square_bb[s];
}
constexpr Bitboard &operator^=(Bitboard &b, const Square s) noexcept {
  return b ^= square_bb[s];
}

//------------------------------------------------
// Additional File operators
//------------------------------------------------
constexpr Bitboard operator&(const Bitboard b, const File f) noexcept {
  return b & FileBB[f];
}
constexpr Bitboard operator|(const Bitboard b, const File f) noexcept {
  return b | FileBB[f];
}
constexpr Bitboard operator^(const Bitboard b, const File f) noexcept {
  return b ^ FileBB[f];
}
constexpr Bitboard &operator|=(Bitboard &b, const File f) noexcept {
  return b |= FileBB[f];
}
constexpr Bitboard &operator^=(Bitboard &b, const File f) noexcept {
  return b ^= FileBB[f];
}

//------------------------------------------------
// Additional Rank operators
//------------------------------------------------
constexpr Bitboard operator&(const Bitboard b, const Rank r) noexcept {
  return b & RankBB[r];
}
constexpr Bitboard operator&(const Rank r, const Bitboard b) noexcept {
  return b & RankBB[r];
}
constexpr Bitboard operator|(const Bitboard b, const Rank r) noexcept {
  return b | RankBB[r];
}
constexpr Bitboard operator^(const Bitboard b, const Rank r) noexcept {
  return b ^ RankBB[r];
}
constexpr Bitboard &operator|=(Bitboard &b, const Rank r) noexcept {
  return b |= RankBB[r];
}
constexpr Bitboard &operator^=(Bitboard &b, const Rank r) noexcept {
  return b ^= RankBB[r];
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

constexpr void init_between_bitboards(const Square from, Bitboard (*step_func)(Bitboard), const Direction step) {
  auto bb      = step_func(bit(from));
  auto to      = from + step;
  auto between = ZeroBB;

  while (bb)
  {
    if (from >= sq_nb || to >= sq_nb)
      continue;

    between_bb[from][to] = between;
    between |= bb;
    bb = step_func(bb);
    to += step;
  }
}

constexpr void init() {
  std::array<std::array<Bitboard, sq_nb>, COL_NB> pawn_east_attack_span{};
  std::array<std::array<Bitboard, sq_nb>, COL_NB> pawn_west_attack_span{};

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

    init_between_bitboards(sq, north_one, NORTH);
    init_between_bitboards(sq, north_east_one, NORTH_EAST);
    init_between_bitboards(sq, east_one, EAST);
    init_between_bitboards(sq, south_east_one, SOUTH_EAST);
    init_between_bitboards(sq, south_one, SOUTH);
    init_between_bitboards(sq, south_west_one, SOUTH_WEST);
    init_between_bitboards(sq, west_one, WEST);
    init_between_bitboards(sq, north_west_one, NORTH_WEST);

    pawn_captures[WHITE][sq] = north_east_one(bbsq) | north_west_one(bbsq);
    pawn_captures[BLACK][sq] = south_east_one(bbsq) | south_west_one(bbsq);
  }
}

constexpr Bitboard (*pawn_push_shift[COL_NB])(Bitboard)   = {north_one, south_one};
constexpr Bitboard (*pawn_east_attacks[COL_NB])(Bitboard) = {north_west_one, south_west_one};
constexpr Bitboard (*pawn_west_attacks[COL_NB])(Bitboard) = {north_east_one, south_east_one};
constexpr Bitboard (*pawn_fill[COL_NB])(Bitboard)         = {north_fill, south_fill};

constexpr Bitboard pawn_push(const Color c, const Bitboard bb) { return pawn_push_shift[c](bb); }

template<Direction D>
constexpr Bitboard pawn_push(const Bitboard bb) {
  static_assert(D != NORTH || D != SOUTH);
  if constexpr (D == NORTH)
    return north_one(bb);
  else
    return south_one(bb);
}

constexpr void reset_lsb(Bitboard &x) {
  x &= (x - 1);
}

constexpr Square lsb(const Bitboard x) {
  return static_cast<Square>(std::countr_zero(x));
}

constexpr Square pop_lsb(Bitboard *b) {
  const auto s = lsb(*b);
  *b &= *b - 1;
  return s;
}

constexpr bool more_than_one(const Bitboard b) {
  return b & (b - 1);
}

constexpr bool is_opposite_colors(const Square s1, const Square s2) {
  return ((static_cast<int>(s1) + static_cast<int>(rank_of(s1)) + s2 + rank_of(s2)) & 1) != 0;
}

constexpr int color_of(const Square square) {
  return static_cast<int>((DarkSquares >> square) & 1);
}

}// namespace bitboard

using namespace bitboard;