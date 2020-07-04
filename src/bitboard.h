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

#include <cassert>
#include <cstdint>
#include <bit>
#include <algorithm>
#include <string_view>

#include "types.h"
#include "util.h"

namespace bitboard {

std::string print_bitboard(Bitboard bb, std::string_view title);
void init();

}// namespace bitboard

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

constexpr Bitboard corner_a1 = make_bitboard(A1, B1, A2, B2);
constexpr Bitboard corner_a8 = make_bitboard(A8, B8, A7, B7);
constexpr Bitboard corner_h1 = make_bitboard(H1, G1, H2, G2);
constexpr Bitboard corner_h8 = make_bitboard(H8, G8, H7, G7);

constexpr std::array<Bitboard, SQ_NB> square_bb{
        make_bitboard(A1), make_bitboard(B1), make_bitboard(C1), make_bitboard(D1), make_bitboard(E1), make_bitboard(F1), make_bitboard(G1), make_bitboard(H1),
        make_bitboard(A2), make_bitboard(B2), make_bitboard(C2), make_bitboard(D2), make_bitboard(E2), make_bitboard(F2), make_bitboard(G2), make_bitboard(H2),
        make_bitboard(A3), make_bitboard(B3), make_bitboard(C3), make_bitboard(D3), make_bitboard(E3), make_bitboard(F3), make_bitboard(G3), make_bitboard(H3),
        make_bitboard(A4), make_bitboard(B4), make_bitboard(C4), make_bitboard(D4), make_bitboard(E4), make_bitboard(F4), make_bitboard(G4), make_bitboard(H4),
        make_bitboard(A5), make_bitboard(B5), make_bitboard(C5), make_bitboard(D5), make_bitboard(E5), make_bitboard(F5), make_bitboard(G5), make_bitboard(H5),
        make_bitboard(A6), make_bitboard(B6), make_bitboard(C6), make_bitboard(D6), make_bitboard(E6), make_bitboard(F6), make_bitboard(G6), make_bitboard(H6),
        make_bitboard(A7), make_bitboard(B7), make_bitboard(C7), make_bitboard(D7), make_bitboard(E7), make_bitboard(F7), make_bitboard(G7), make_bitboard(H7),
        make_bitboard(A8), make_bitboard(B8), make_bitboard(C8), make_bitboard(D8), make_bitboard(E8), make_bitboard(F8), make_bitboard(G8), make_bitboard(H8)};

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

consteval std::array<Bitboard, SQ_NB> make_knight_attacks()
{
  std::array<Bitboard, SQ_NB> result{};
  for (const auto s : Squares)
  {
    const auto bbsq = square_bb[s];
    result[s] =  (bbsq & ~(FileABB | FileBBB)) << 6;
    result[s] |= (bbsq & ~FileABB) << 15;
    result[s] |= (bbsq & ~FileHBB) << 17;
    result[s] |= (bbsq & ~(FileGBB | FileHBB)) << 10;
    result[s] |= (bbsq & ~(FileGBB | FileHBB)) >> 6;
    result[s] |= (bbsq & ~FileHBB) >> 15;
    result[s] |= (bbsq & ~FileABB) >> 17;
    result[s] |= (bbsq & ~(FileABB | FileBBB)) >> 10;
  }

  return result;
}

constexpr std::array<Bitboard, SQ_NB> knight_attacks = make_knight_attacks();

consteval std::array<Bitboard, SQ_NB> make_king_attacks()
{
  std::array<Bitboard, SQ_NB> result{};
  for (const auto s : Squares)
  {
    const auto bbsq = square_bb[s];
    result[s] =  (bbsq & ~FileABB) >> 1;
    result[s] |= (bbsq & ~FileABB) << 7;
    result[s] |= bbsq << 8;
    result[s] |= (bbsq & ~FileHBB) << 9;
    result[s] |= (bbsq & ~FileHBB) << 1;
    result[s] |= (bbsq & ~FileHBB) >> 7;
    result[s] |= bbsq >> 8;
    result[s] |= (bbsq & ~FileABB) >> 9;
  }

  return result;
}

constexpr std::array<Bitboard, SQ_NB> king_attacks = make_king_attacks();

template<typename T1 = Square>
constexpr int distance(const Square x, const Square y);
template<>
constexpr int distance<File>(const Square x, const Square y) { return util::abs(file_of(x) - file_of(y)); }
template<>
constexpr int distance<Rank>(const Square x, const Square y) { return util::abs(rank_of(x) - rank_of(y)); }

inline Bitboard between_bb[SQ_NB][SQ_NB];
inline Bitboard passed_pawn_front_span[COL_NB][SQ_NB];
inline Bitboard pawn_front_span[COL_NB][SQ_NB];
inline Bitboard pawn_captures[COL_NB][SQ_NB];
inline std::array<Square, 2> oo_king_from{};
inline std::array<Square, 2> ooo_king_from{};

/// indexed by the position of the king
inline std::array<Square, SQ_NB> rook_castles_to{};

/// indexed by the position of the king
inline std::array<Square, SQ_NB> rook_castles_from{};

consteval std::array<std::array<int, SQ_NB>, SQ_NB> make_distance()
{
  std::array<std::array<int, SQ_NB>, SQ_NB> result{};
  for (const auto sq1 : Squares)
    for (const auto sq2 : Squares)
      result[sq1][sq2] = std::max(distance<Rank>(sq1, sq2), distance<File>(sq1, sq2));

  return result;
}

constexpr std::array<std::array<int, SQ_NB>, SQ_NB> dist = make_distance(); /// chebyshev distance

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
constexpr Bitboard bit(const Rank r, Ranks... ranks) {
  return RankBB[r] | bit(ranks...);
}

template<typename... Files>
constexpr Bitboard bit(const File f, Files... files) {
  return FileBB[f] | bit(files...);
}

constexpr Bitboard bb_rank(const Rank r) {
  return RankBB[r];
}

constexpr Bitboard bb_file(const File f) {
  return FileBB[f];
}

constexpr Bitboard bb_file(const Square s) {
  return bb_file(file_of(s));
}

//------------------------------------------------
// Additional Square operators
//------------------------------------------------
constexpr Bitboard operator&(const Bitboard bb, const Square s) noexcept {
  return bb & square_bb[s];
}
constexpr Bitboard operator|(const Bitboard bb, const Square s) noexcept {
  return bb | square_bb[s];
}
constexpr Bitboard operator^(const Bitboard bb, const Square s) noexcept {
  return bb ^ square_bb[s];
}
constexpr Bitboard &operator|=(Bitboard &bb, const Square s) noexcept {
  return bb |= square_bb[s];
}
constexpr Bitboard &operator^=(Bitboard &bb, const Square s) noexcept {
  return bb ^= square_bb[s];
}

//------------------------------------------------
// Additional File operators
//------------------------------------------------
constexpr Bitboard operator&(const Bitboard bb, const File f) noexcept {
  return bb & FileBB[f];
}
constexpr Bitboard operator|(const Bitboard bb, const File f) noexcept {
  return bb | FileBB[f];
}
constexpr Bitboard operator^(const Bitboard bb, const File f) noexcept {
  return bb ^ FileBB[f];
}
constexpr Bitboard &operator|=(Bitboard &bb, const File f) noexcept {
  return bb |= FileBB[f];
}
constexpr Bitboard &operator^=(Bitboard &bb, const File f) noexcept {
  return bb ^= FileBB[f];
}

//------------------------------------------------
// Additional Rank operators
//------------------------------------------------
constexpr Bitboard operator&(const Bitboard bb, const Rank r) noexcept {
  return bb & RankBB[r];
}
constexpr Bitboard operator&(const Rank r, const Bitboard bb) noexcept {
  return bb & RankBB[r];
}
constexpr Bitboard operator|(const Bitboard bb, const Rank r) noexcept {
  return bb | RankBB[r];
}
constexpr Bitboard operator^(const Bitboard bb, const Rank r) noexcept {
  return bb ^ RankBB[r];
}
constexpr Bitboard &operator|=(Bitboard &bb, const Rank r) noexcept {
  return bb |= RankBB[r];
}
constexpr Bitboard &operator^=(Bitboard &bb, const Rank r) noexcept {
  return bb ^= RankBB[r];
}

template<Direction D>
constexpr Bitboard shift_bb(const Bitboard bb)
{
  if constexpr (D == NORTH)
    return bb << 8;
  else if constexpr (D == SOUTH)
    return bb >> 8;
  else if constexpr (D == EAST)
    return (bb & ~FileHBB) << 1;
  else if constexpr (D == WEST)
    return (bb & ~FileABB) >> 1;
  else if constexpr (D == NORTH_EAST)
    return (bb & ~FileHBB) << 9;
  else if constexpr (D == SOUTH_EAST)
    return (bb & ~FileHBB) >> 7;
  else if constexpr (D == SOUTH_WEST)
    return (bb & ~FileABB) >> 9;
  else if constexpr (D == NORTH_WEST)
    return (bb & ~FileABB) << 7;

  assert(false);
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
constexpr Bitboard (*pawn_east_attacks[COL_NB])(Bitboard) = {shift_bb<NORTH_WEST>, shift_bb<SOUTH_WEST>};
constexpr Bitboard (*pawn_west_attacks[COL_NB])(Bitboard) = {shift_bb<NORTH_EAST>, shift_bb<SOUTH_EAST>};
constexpr Bitboard (*pawn_fill[COL_NB])(Bitboard) = {north_fill, south_fill};

constexpr Bitboard pawn_push(const Color c, const Bitboard bb) { return c == WHITE ? shift_bb<NORTH>(bb) : shift_bb<SOUTH>(bb); }

constexpr void reset_lsb(Bitboard &bb) {
    bb &= (bb - 1);
}

constexpr Square lsb(const Bitboard bb) {
  return static_cast<Square>(std::countr_zero(bb));
}

constexpr Square pop_lsb(Bitboard *bb) {
  const auto s = lsb(*bb);
  *bb &= *bb - 1;
  return s;
}

constexpr bool more_than_one(const Bitboard bb) {
  return bb & (bb - 1);
}

constexpr bool is_opposite_colors(const Square s1, const Square s2) {
  return ((static_cast<int>(s1) + static_cast<int>(rank_of(s1)) + s2 + rank_of(s2)) & 1) != 0;
}
