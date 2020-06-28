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

#include <fmt/format.h>

#include "board.h"
#include "magic.h"

void Board::clear() {
  piece.fill(0);
  occupied_by_side.fill(0);
  board.fill(NoPiece);
  king_square.fill(no_square);
  occupied = 0;
}

void Board::make_move(const Move m) {

  const auto from = move_from(m);
  const auto to   = move_to(m);
  const auto pc   = move_piece(m);

  if (!is_castle_move(m))
  {
    remove_piece(from);

    if (is_ep_capture(m))
    {
      const auto direction = pawn_push(color_of(pc));
      remove_piece(to - direction);
    } else if (is_capture(m))
      remove_piece(to);

    if (is_promotion(m))
      add_piece(move_promoted(m), to);
    else
      add_piece(pc, to);
  } else
  {
    const auto rook = make_piece(Rook, move_side(m));
    remove_piece(rook_castles_from[to]);
    remove_piece(from);
    add_piece(rook, rook_castles_to[to]);
    add_piece(pc, to);
  }

  if (type_of(pc) == King)
    king_square[move_side(m)] = to;
}

void Board::unmake_move(const Move m) {

  const auto from = move_from(m);
  const auto to   = move_to(m);
  const auto pc   = move_piece(m);

  if (!is_castle_move(m))
  {
    if (is_promotion(m))
      remove_piece(to);
    else
      remove_piece(to);

    if (is_ep_capture(m))
    {
      const auto direction = pawn_push(color_of(pc));
      add_piece(move_captured(m), to - direction);
    } else if (is_capture(m))
      add_piece(move_captured(m), to);

    add_piece(pc, from);
  } else
  {
    const auto rook = make_piece(Rook, move_side(m));
    remove_piece(to);
    remove_piece(rook_castles_to[to]);
    add_piece(pc, from);
    add_piece(rook, rook_castles_from[to]);
  }

  if (type_of(pc) == King)
    king_square[move_side(m)] = from;
}

Bitboard Board::get_pinned_pieces(const Color side, const Square sq) {
  const auto them       = ~side;
  auto pinners          = xray_bishop_attacks(occupied, occupied_by_side[side], sq) & (piece[make_piece(Bishop, them)] | piece[make_piece(Queen, them)]);
  auto pinned_pieces    = ZeroBB;

  while (pinners)
    pinned_pieces |= between_bb[pop_lsb(&pinners)][sq] & occupied_by_side[side];

  pinners = xray_rook_attacks(occupied, occupied_by_side[side], sq) & (piece[make_piece(Rook, them)] | piece[make_piece(Queen, them)]);

  while (pinners)
    pinned_pieces |= between_bb[pop_lsb(&pinners)][sq] & occupied_by_side[side];

  return pinned_pieces;
}

bool Board::is_attacked_by_slider(const Square sq, const Color side) const {
  const auto r_attacks = piece_attacks_bb<Rook>(sq, occupied);

  if (pieces(Rook, side) & r_attacks)
    return true;

  const auto b_attacks = piece_attacks_bb<Bishop>(sq, occupied);

  if (pieces(Bishop, side) & b_attacks)
    return true;

  return (pieces(Queen, side) & (b_attacks | r_attacks)) != 0;
}

void Board::print() const {
  constexpr std::string_view piece_letter = "PNBRQK. pnbrqk. ";

  fmt::memory_buffer s;

  fmt::format_to(s, "\n");

  for (const Rank rank : ReverseRanks)
  {
    fmt::format_to(s, "{}  ", rank + 1);

    for (const auto file : Files)
    {
      const auto sq = make_square(file, rank);
      const auto pc = get_piece(sq);
      fmt::format_to(s, "{} ", piece_letter[pc]);
    }
    fmt::format_to(s, "\n");
  }

  fmt::print("{}   a b c d e f g h\n", fmt::to_string(s));
}

bool Board::is_passed_pawn_move(const Move m) const {
  return move_piece_type(m) == Pawn && is_pawn_passed(move_to(m), move_side(m));
}

bool Board::is_pawn_isolated(const Square sq, const Color side) const {
  const auto f               = bb_file(file_of(sq));
  const auto neighbour_files = east_one(f) | west_one(f);
  return (pieces(Pawn, side) & neighbour_files) == 0;
}

bool Board::is_pawn_behind(const Square sq, const Color side) const {
  const auto bbsq = bit(sq);
  return (pieces(Pawn, side) & pawn_fill[~side](west_one(bbsq) | east_one(bbsq))) == 0;
}
