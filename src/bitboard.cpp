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
#include <string_view>

#include <fmt/format.h>

#include "bitboard.h"
#include "types.h"

namespace {

template<Direction Step>
constexpr void init_between_bitboards(const Square from) {
  auto bb      = shift_bb<Step>(bit(from));
  auto to      = from + Step;
  auto between = ZeroBB;

  while (bb)
  {
    if (from >= sq_nb || to >= sq_nb)
      continue;

    between_bb[from][to] = between;
    between |= bb;
    bb = shift_bb<Step>(bb);
    to += Step;
  }
}
}// namespace

std::string bitboard::print_bitboard(Bitboard b, std::string_view title) {

  fmt::memory_buffer buffer;

  constexpr std::string_view line = "+---+---+---+---+---+---+---+---+";

  if (!title.empty())
    fmt::format_to(buffer, "{}\n", title);

  fmt::format_to(buffer, "{}\n", line);

  for (const auto r : ReverseRanks)
  {
    for (const auto f : Files)
      fmt::format_to(buffer, "| {} ", b & make_square(f, r) ? "X" : " ");

    fmt::format_to(buffer, "| {}\n{}\n", std::to_string(1 + r), line);
  }
  fmt::format_to(buffer, "  a   b   c   d   e   f   g   h\n");

  return fmt::to_string(buffer);
}

void bitboard::init() {
  std::array<std::array<Bitboard, sq_nb>, COL_NB> pawn_east_attack_span{};
  std::array<std::array<Bitboard, sq_nb>, COL_NB> pawn_west_attack_span{};

  for (const auto sq : Squares)
  {
    const auto bbsq = square_bb[sq];

    pawn_front_span[WHITE][sq]        = north_fill(shift_bb<NORTH     >(bbsq));
    pawn_front_span[BLACK][sq]        = south_fill(shift_bb<SOUTH     >(bbsq));
    pawn_east_attack_span[WHITE][sq]  = north_fill(shift_bb<NORTH_WEST>(bbsq));
    pawn_east_attack_span[BLACK][sq]  = south_fill(shift_bb<SOUTH_EAST>(bbsq));
    pawn_west_attack_span[WHITE][sq]  = north_fill(shift_bb<NORTH_WEST>(bbsq));
    pawn_west_attack_span[BLACK][sq]  = south_fill(shift_bb<WEST      >(bbsq));
    passed_pawn_front_span[WHITE][sq] = pawn_east_attack_span[WHITE][sq] | pawn_front_span[WHITE][sq] | pawn_west_attack_span[WHITE][sq];
    passed_pawn_front_span[BLACK][sq] = pawn_east_attack_span[BLACK][sq] | pawn_front_span[BLACK][sq] | pawn_west_attack_span[BLACK][sq];

    std::fill(std::begin(between_bb[sq]), std::end(between_bb[sq]), 0);
    init_between_bitboards<NORTH     >(sq);
    init_between_bitboards<NORTH_EAST>(sq);
    init_between_bitboards<EAST      >(sq);
    init_between_bitboards<SOUTH_EAST>(sq);
    init_between_bitboards<SOUTH     >(sq);
    init_between_bitboards<SOUTH_WEST>(sq);
    init_between_bitboards<WEST      >(sq);
    init_between_bitboards<NORTH_WEST>(sq);

    pawn_captures[WHITE][sq] = shift_bb<NORTH_EAST>(bbsq) | shift_bb<NORTH_WEST>(bbsq);
    pawn_captures[BLACK][sq] = shift_bb<SOUTH_EAST>(bbsq) | shift_bb<SOUTH_WEST>(bbsq);
  }
}
