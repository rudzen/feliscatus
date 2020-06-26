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

#include "board.h"
#include "material.h"
#include "bitboard.h"
#include "magic.h"
#include "types.h"

namespace {

constexpr int SEE_INVALID_SCORE = -5000;

constexpr int material_change(const Move move) {
  return (is_capture(move) ? piece_value(move_captured(move)) : 0) + (is_promotion(move) ? (piece_value(move_promoted(move)) - piece_value(Pawn)) : 0);
}

constexpr Piece next_to_capture(const Move move) {
  return is_promotion(move) ? move_promoted(move) : move_piece(move);
}

}

int Board::see_move(Move move) {
  int score;
  make_move(move);

  const auto us   = move_side(move);
  const auto them = ~us;

  if (!is_attacked(king_square[us], them))
  {
    init_see_move();
    score = see_rec(material_change(move), next_to_capture(move), move_to(move), them);
  } else
    score = SEE_INVALID_SCORE;

  unmake_move(move);
  return score;
}

int Board::see_last_move(const Move move) {
  init_see_move();
  return see_rec(material_change(move), next_to_capture(move), move_to(move), ~move_side(move));
}

int Board::see_rec(const int mat_change, const Piece next_capture, const Square to, const Color side_to_move) {
  const auto rr = relative_rank(side_to_move, to);

  Move move;

  do
  {
    const auto from = lookup_best_attacker(to, side_to_move);
    if (!from)
      return mat_change;

    const auto current_pt = current_piece[side_to_move];

    move = current_pt == Pawn && rr == RANK_8
         ? init_move<PROMOTION | CAPTURE>(make_piece(current_pt, side_to_move), next_capture, from.value(), to, make_piece(Queen, side_to_move))
         : init_move<CAPTURE>(make_piece(current_pt, side_to_move), next_capture, from.value(), to, NoPiece);

    make_move(move);

    if (!is_attacked(king_square[side_to_move], ~side_to_move))
      break;

    unmake_move(move);
  } while (true);

  const auto score = -see_rec(material_change(move), next_to_capture(move), move_to(move), ~move_side(move));

  unmake_move(move);

  return score < 0 ? mat_change + score : mat_change;
}

std::optional<Square> Board::lookup_best_attacker(const Square to, const Color side) {
  // "Best" == "Lowest piece value"

  Bitboard b;

  const auto get_from_sq = [&](const Bitboard bb)
  {
    const auto from = lsb(bb);
    current_piece_bitboard[side] &= ~bit(from);
    return std::optional<Square>(from);
  };

  switch (current_piece[side])
  {
  case Pawn:
    b = current_piece_bitboard[side] & pawn_attacks_bb(~side, to);
    if (b)
      return get_from_sq(b);
    ++current_piece[side];
    current_piece_bitboard[side] = pieces(Knight, side);
    [[fallthrough]];
  case Knight:
    b = current_piece_bitboard[side] & piece_attacks_bb<Knight>(to);
    if (b)
      return get_from_sq(b);
    ++current_piece[side];
    current_piece_bitboard[side] = pieces(Bishop, side);
    [[fallthrough]];

  case Bishop:
    b = current_piece_bitboard[side] & piece_attacks_bb<Bishop>(to, occupied);
    if (b)
      return get_from_sq(b);
    ++current_piece[side];
    current_piece_bitboard[side] = pieces(Rook, side);
    [[fallthrough]];
  case Rook:
    b = current_piece_bitboard[side] & piece_attacks_bb<Rook>(to, occupied);
    if (b)
      return get_from_sq(b);
    ++current_piece[side];
    current_piece_bitboard[side] = pieces(Queen, side);
    [[fallthrough]];
  case Queen:
    b = current_piece_bitboard[side] & piece_attacks_bb<Queen>(to, occupied);
    if (b)
      return get_from_sq(b);
    ++current_piece[side];
    current_piece_bitboard[side] = pieces(King, side);
    [[fallthrough]];
  case King:
    b = current_piece_bitboard[side] & piece_attacks_bb<King>(to);
    if (b)
      return get_from_sq(b);
    break;
  default:
    break;
  }

  return std::nullopt;

}

void Board::init_see_move() {
  current_piece.fill(Pawn);
  current_piece_bitboard[WHITE] = pieces(Pawn, WHITE);
  current_piece_bitboard[BLACK] = pieces(Pawn, BLACK);
}
