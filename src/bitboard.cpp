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

#include <string>
#include <vector>

#include <fmt/format.h>

#include "bitboard.hpp"

namespace {

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

struct MagicInit final {
  Bitboard magic{};
  int32_t index{};
};

/// set_bit() set a bit at specific location

constexpr void set_bit(Bitboard &bb, const int b) {
  bb |= square_bb[static_cast<std::size_t>(b)];
}


/// test_bit() tests if a bit is set

constexpr bool test_bit(const Bitboard bb, const int b) {
  return (bb & square_bb[static_cast<std::size_t>(b)]) != 0;
}

std::array<Bitboard, 97264> lookupTable;

template<PieceType Pt>
void initialize_magics(const std::array<MagicInit, 64> &magicInit, MagicTable &magic) {

  static_assert(Pt != ROOK || Pt != BISHOP);

  constexpr std::array<std::array<Direction, 2>, 4> BishopDirections{{{SOUTH_WEST, SOUTH_SOUTH_WEST}, {SOUTH_EAST, SOUTH_SOUTH_EAST}, {NORTH_WEST, NORTH_NORTH_WEST}, {NORTH_EAST, NORTH_NORTH_EAST}}};
  constexpr std::array<std::array<Direction, 2>, 4> RookDirections{{{SOUTH, SOUTH * 2}, {WEST, WEST}, {EAST, EAST}, {NORTH, NORTH * 2}}};

  constexpr auto rook_mask = [](const Square sq) {
    auto result   = ZeroBB;
    const auto rk = rank_of(sq);
    const auto fl = file_of(sq);
    for (auto r = rk + 1; r <= 6; ++r)
      result |= square_bb[fl + r * 8];
    for (auto r = rk - 1; r >= 1; --r)
      result |= square_bb[fl + r * 8];
    for (auto f = fl + 1; f <= 6; ++f)
      result |= square_bb[f + rk * 8];
    for (auto f = fl - 1; f >= 1; --f)
      result |= square_bb[f + rk * 8];
    return result;
  };

  constexpr auto bishop_mask = [](const Square sq) {
    auto result   = ZeroBB;
    const auto rk = rank_of(sq);
    const auto fl = file_of(sq);
    for (auto r = rk + 1, f = fl + 1; r <= 6 && f <= 6; ++r, ++f)
      result |= square_bb[f + r * 8];
    for (auto r = rk + 1, f = fl - 1; r <= 6 && f >= 1; ++r, --f)
      result |= square_bb[f + r * 8];
    for (auto r = rk - 1, f = fl + 1; r >= 1 && f <= 6; --r, ++f)
      result |= square_bb[f + r * 8];
    for (auto r = rk - 1, f = fl - 1; r >= 1 && f >= 1; --r, --f)
      result |= square_bb[f + r * 8];
    return result;
  };

  constexpr auto mask       = Pt == ROOK ? rook_mask : bishop_mask;
  constexpr auto shift      = 64 - (Pt == ROOK ? 12 : 9);
  constexpr auto directions = Pt == ROOK ? RookDirections : BishopDirections;

  std::vector<int> squares(SQ_NB / 2);

  for (const auto sq : Squares)
  {
    magic[sq].magic = magicInit[sq].magic;
    magic[sq].data  = &lookupTable[magicInit[sq].index];
    auto bb = magic[sq].mask = mask(sq);
    const auto sq88          = sq + (sq & ~7);

    squares.clear();
    while (bb)
      squares.emplace_back(pop_lsb(&bb));

    // Loop through all possible occupations within the mask and calculate the corresponding attack sets.
    for (Bitboard k = 0; k < (OneBB << squares.size()); ++k)
    {
      auto bb2 = bb = 0;

      for (auto j = 0; j < static_cast<int>(squares.size()); ++j)
        if (test_bit(k, j))
          set_bit(bb, squares[j]);

      for (const auto &direction : directions)
      {
        for (auto d = 1; !((sq88 + d * direction[1]) & 0x88); ++d)
        {
          const auto s = sq + d * direction[0];
          bb2 |= s;
          if (bb & s)
            break;
        }
      }

      const auto l      = ((bb * magic[sq].magic) >> shift);
      magic[sq].data[l] = bb2;
    }
  }
}

}// namespace

namespace bitboard {

std::string print_bitboard(Bitboard bb, std::string_view title) {

  fmt::memory_buffer buffer;

  constexpr std::string_view line = "+---+---+---+---+---+---+---+---+";

  if (!title.empty())
    fmt::format_to(buffer, "{}\n", title);

  fmt::format_to(buffer, "{}\n", line);

  for (const auto r : ReverseRanks)
  {
    for (const auto f : Files)
      fmt::format_to(buffer, "| {} ", bb & make_square(f, r) ? "X" : " ");

    fmt::format_to(buffer, "| {}\n{}\n", std::to_string(1 + r), line);
  }
  fmt::format_to(buffer, "  a   b   c   d   e   f   g   h\n");

  return fmt::to_string(buffer);
}

void init() {

  static constexpr std::array<PieceType,  2> MinorSliders{BISHOP, ROOK};
  static constexpr std::array<MagicInit, SQ_NB> BishopInit{
    {{0x007bfeffbfeffbff, 16530}, {0x003effbfeffbfe08,  9162}, {0x0000401020200000,  9674}, {0x0000200810000000, 18532}, {0x0000110080000000, 19172}, {0x0000080100800000, 17700},
     {0x0007efe0bfff8000,  5730}, {0x00000fb0203fff80, 19661}, {0x00007dff7fdff7fd, 17065}, {0x0000011fdff7efff, 12921}, {0x0000004010202000, 15683}, {0x0000002008100000, 17764},
     {0x0000001100800000, 19684}, {0x0000000801008000, 18724}, {0x000007efe0bfff80,  4108}, {0x000000080f9fffc0, 12936}, {0x0000400080808080, 15747}, {0x0000200040404040,  4066},
     {0x0000400080808080, 14359}, {0x0000200200801000, 36039}, {0x0000240080840000, 20457}, {0x0000080080840080, 43291}, {0x0000040010410040,  5606}, {0x0000020008208020,  9497},
     {0x0000804000810100, 15715}, {0x0000402000408080, 13388}, {0x0000804000810100,  5986}, {0x0000404004010200, 11814}, {0x0000404004010040, 92656}, {0x0000101000804400,  9529},
     {0x0000080800104100, 18118}, {0x0000040400082080,  5826}, {0x0000410040008200,  4620}, {0x0000208020004100, 12958}, {0x0000110080040008, 55229}, {0x0000020080080080,  9892},
     {0x0000404040040100, 33767}, {0x0000202040008040, 20023}, {0x0000101010002080,  6515}, {0x0000080808001040,  6483}, {0x0000208200400080, 19622}, {0x0000104100200040,  6274},
     {0x0000208200400080, 18404}, {0x0000008840200040, 14226}, {0x0000020040100100, 17990}, {0x007fff80c0280050, 18920}, {0x0000202020200040, 13862}, {0x0000101010100020, 19590},
     {0x0007ffdfc17f8000,  5884}, {0x0003ffefe0bfc000, 12946}, {0x0000000820806000,  5570}, {0x00000003ff004000, 18740}, {0x0000000100202000,  6242}, {0x0000004040802000, 12326},
     {0x007ffeffbfeff820,  4156}, {0x003fff7fdff7fc10, 12876}, {0x0003ffdfdfc27f80, 17047}, {0x000003ffefe0bfc0, 17780}, {0x0000000008208060,  2494}, {0x0000000003ff0040, 17716},
     {0x0000000001002020, 17067}, {0x0000000040408020,  9465}, {0x00007ffeffbfeff9, 16196}, {0x007ffdff7fdff7fd,  6166}}};
  static constexpr std::array<MagicInit, SQ_NB> RookInit{
    {{0x00a801f7fbfeffff, 85487}, {0x00180012000bffff, 43101}, {0x0040080010004004,     0}, {0x0040040008004002, 49085}, {0x0040020004004001, 93168}, {0x0020008020010202, 78956},
     {0x0040004000800100, 60703}, {0x0810020990202010, 64799}, {0x000028020a13fffe, 30640}, {0x003fec008104ffff,  9256}, {0x00001800043fffe8, 28647}, {0x00001800217fffe8, 10404},
     {0x0000200100020020, 63775}, {0x0000200080010020, 14500}, {0x0000300043ffff40, 52819}, {0x000038010843fffd,  2048}, {0x00d00018010bfff8, 52037}, {0x0009000c000efffc, 16435},
     {0x0004000801020008, 29104}, {0x0002002004002002, 83439}, {0x0001002002002001, 86842}, {0x0001001000801040, 27623}, {0x0000004040008001, 26599}, {0x0000802000200040, 89583},
     {0x0040200010080010,  7042}, {0x0000080010040010, 84463}, {0x0004010008020008, 82415}, {0x0000020020040020, 95216}, {0x0000010020020020, 35015}, {0x0000008020010020, 10790},
     {0x0000008020200040, 53279}, {0x0000200020004081, 70684}, {0x0040001000200020, 38640}, {0x0000080400100010, 32743}, {0x0004010200080008, 68894}, {0x0000200200200400, 62751},
     {0x0000200100200200, 41670}, {0x0000200080200100, 25575}, {0x0000008000404001,  3042}, {0x0000802000200040, 36591}, {0x00ffffb50c001800, 69918}, {0x007fff98ff7fec00,  9092},
     {0x003ffff919400800, 17401}, {0x001ffff01fc03000, 40688}, {0x0000010002002020, 96240}, {0x0000008001002020, 91632}, {0x0003fff673ffa802, 32495}, {0x0001fffe6fff9001, 51133},
     {0x00ffffd800140028, 78319}, {0x007fffe87ff7ffec, 12595}, {0x003fffd800408028,  5152}, {0x001ffff111018010, 32110}, {0x000ffff810280028, 13894}, {0x0007fffeb7ff7fd8,  2546},
     {0x0003fffc0c480048, 41052}, {0x0001ffffa2280028, 77676}, {0x00ffffe4ffdfa3ba, 73580}, {0x007ffb7fbfdfeff6, 44947}, {0x003fffbfdfeff7fa, 73565}, {0x001fffeff7fbfc22, 17682},
     {0x000ffffbf7fc2ffe, 56607}, {0x0007fffdfa03ffff, 56135}, {0x0003ffdeff7fbdec, 44989}, {0x0001ffff99ffab2f, 21479}}};

  initialize_magics<BISHOP>(BishopInit, MagicTables[BISHOP - 2]);
  initialize_magics<ROOK>(RookInit, MagicTables[ROOK - 2]);

  // knight and king attacks are generated at compile-time, so just copy
  AllAttacks[KNIGHT] = knight_attacks;
  AllAttacks[KING] = king_attacks;

  for (const auto s1 : Squares)
  {
    AllAttacks[QUEEN][s1] = AllAttacks[BISHOP][s1] = piece_attacks_bb<BISHOP>(s1, 0);
    AllAttacks[QUEEN][s1] |= AllAttacks[ROOK][s1]  = piece_attacks_bb<  ROOK>(s1, 0);

    for (const auto pt : MinorSliders)
      for (const auto s2 : Squares)
      {
        if (!(AllAttacks[pt][s1] & s2))
          continue;

        Lines[s1][s2] = (piece_attacks_bb(pt, s1, 0) & piece_attacks_bb(pt, s2, 0)) | s1 | s2;
      }
  }

  std::array<std::array<Bitboard, SQ_NB>, COL_NB> pawn_east_attack_span{};
  std::array<std::array<Bitboard, SQ_NB>, COL_NB> pawn_west_attack_span{};

  for (const auto s : Squares)
  {
    const auto bb = square_bb[s];

    pawn_front_span[WHITE][s]        = fill<NORTH>(shift_bb<NORTH     >(bb));
    pawn_front_span[BLACK][s]        = fill<SOUTH>(shift_bb<SOUTH>(bb));
    pawn_east_attack_span[WHITE][s]  = fill<NORTH>(shift_bb<NORTH_WEST>(bb));
    pawn_east_attack_span[BLACK][s]  = fill<SOUTH>(shift_bb<SOUTH_EAST>(bb));
    pawn_west_attack_span[WHITE][s]  = fill<NORTH>(shift_bb<NORTH_WEST>(bb));
    pawn_west_attack_span[BLACK][s]  = fill<SOUTH>(shift_bb<WEST>(bb));
    passed_pawn_front_span[WHITE][s] = pawn_east_attack_span[WHITE][s] | pawn_front_span[WHITE][s] | pawn_west_attack_span[WHITE][s];
    passed_pawn_front_span[BLACK][s] = pawn_east_attack_span[BLACK][s] | pawn_front_span[BLACK][s] | pawn_west_attack_span[BLACK][s];

    pawn_captures[WHITE][s] = shift_bb<NORTH_EAST>(bb) | shift_bb<NORTH_WEST>(bb);
    pawn_captures[BLACK][s] = shift_bb<SOUTH_EAST>(bb) | shift_bb<SOUTH_WEST>(bb);
  }
}

}
