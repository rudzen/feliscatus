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

#include <fmt/format.h>

#include "bitboard.h"

namespace {

template<Direction Step>
constexpr void init_between_bitboards(const Square from) {
  auto bb      = shift_bb<Step>(bit(from));
  auto to      = from + Step;
  auto between = ZeroBB;

  while (bb)
  {
    if (from >= SQ_NB || to >= SQ_NB)
      continue;

    between_bb[from][to] = between;
    between |= bb;
    bb = shift_bb<Step>(bb);
    to += Step;
  }
}
}// namespace

std::string bitboard::print_bitboard(Bitboard bb, std::string_view title) {

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

void bitboard::init() {
  std::array<std::array<Bitboard, SQ_NB>, COL_NB> pawn_east_attack_span{};
  std::array<std::array<Bitboard, SQ_NB>, COL_NB> pawn_west_attack_span{};

  for (const auto s : Squares)
  {
    const auto bb = square_bb[s];

    pawn_front_span[WHITE][s]        = north_fill(shift_bb<NORTH     >(bb));
    pawn_front_span[BLACK][s]        = south_fill(shift_bb<SOUTH     >(bb));
    pawn_east_attack_span[WHITE][s]  = north_fill(shift_bb<NORTH_WEST>(bb));
    pawn_east_attack_span[BLACK][s]  = south_fill(shift_bb<SOUTH_EAST>(bb));
    pawn_west_attack_span[WHITE][s]  = north_fill(shift_bb<NORTH_WEST>(bb));
    pawn_west_attack_span[BLACK][s]  = south_fill(shift_bb<WEST      >(bb));
    passed_pawn_front_span[WHITE][s] = pawn_east_attack_span[WHITE][s] | pawn_front_span[WHITE][s] | pawn_west_attack_span[WHITE][s];
    passed_pawn_front_span[BLACK][s] = pawn_east_attack_span[BLACK][s] | pawn_front_span[BLACK][s] | pawn_west_attack_span[BLACK][s];

    std::fill(std::begin(between_bb[s]), std::end(between_bb[s]), 0);
    init_between_bitboards<NORTH     >(s);
    init_between_bitboards<NORTH_EAST>(s);
    init_between_bitboards<EAST      >(s);
    init_between_bitboards<SOUTH_EAST>(s);
    init_between_bitboards<SOUTH     >(s);
    init_between_bitboards<SOUTH_WEST>(s);
    init_between_bitboards<WEST      >(s);
    init_between_bitboards<NORTH_WEST>(s);

    pawn_captures[WHITE][s] = shift_bb<NORTH_EAST>(bb) | shift_bb<NORTH_WEST>(bb);
    pawn_captures[BLACK][s] = shift_bb<SOUTH_EAST>(bb) | shift_bb<SOUTH_WEST>(bb);
  }

  for (const auto side : Colors)
  {
    const auto rank1                            = relative_rank(side, RANK_1);
    rook_castles_to[make_square(FILE_G, rank1)] = make_square(FILE_F, rank1);
    rook_castles_to[make_square(FILE_C, rank1)] = make_square(FILE_D, rank1);
  }
}
