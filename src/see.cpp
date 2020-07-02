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
#include "bitboard.h"
#include "magic.h"
#include "types.h"

namespace {

using CurrentPieceType = std::array<PieceType, COL_NB>;
using CurrentPieces = std::array<Bitboard, COL_NB>;

struct SeeData final {
  CurrentPieceType current_pt{};
  CurrentPieces current_pc{};
};

constexpr int SEE_INVALID_SCORE = -5000;

constexpr auto material_change(const Move move) {
  return (is_capture(move) ? piece_value(move_captured(move)) : 0) + (is_promotion(move) ? (piece_value(move_promoted(move)) - piece_value(Pawn)) : 0);
}

constexpr auto next_to_capture(const Move move) {
  return is_promotion(move) ? move_promoted(move) : move_piece(move);
}

auto get_from_sq (const Bitboard bb, SeeData &data, const Color stm) {
  const auto from = lsb(bb);
  data.current_pc[stm] &= ~bit(from);
  return std::make_optional(from);
}

std::optional<Square> lookup_best_attacker(SeeData &data, const Square to, const Color side, const Board *b) {
  // "Best" == "Lowest piece value"

  const auto occupied = b->pieces();
  Bitboard bb;

  switch (data.current_pt[side])
  {
  case Pawn:
    bb = data.current_pc[side] & pawn_attacks_bb(~side, to);
    if (bb)
      return get_from_sq(bb, data, side);
    ++data.current_pt[side];
    data.current_pc[side] = b->pieces(Knight, side);
    [[fallthrough]];
  case Knight:
    bb = data.current_pc[side] & piece_attacks_bb<Knight>(to);
    if (bb)
      return get_from_sq(bb, data, side);
    ++data.current_pt[side];
    data.current_pc[side] = b->pieces(Bishop, side);
    [[fallthrough]];

  case Bishop:
    bb = data.current_pc[side] & piece_attacks_bb<Bishop>(to, occupied);
    if (bb)
      return get_from_sq(bb, data, side);
    ++data.current_pt[side];
    data.current_pc[side] = b->pieces(Rook, side);
    [[fallthrough]];
  case Rook:
    bb = data.current_pc[side] & piece_attacks_bb<Rook>(to, occupied);
    if (bb)
      return get_from_sq(bb, data, side);
    ++data.current_pt[side];
    data.current_pc[side] = b->pieces(Queen, side);
    [[fallthrough]];
  case Queen:
    bb = data.current_pc[side] & piece_attacks_bb<Queen>(to, occupied);
    if (bb)
      return get_from_sq(bb, data, side);
    ++data.current_pt[side];
    data.current_pc[side] = b->pieces(King, side);
    [[fallthrough]];
  case King:
    bb = data.current_pc[side] & piece_attacks_bb<King>(to);
    if (bb)
      return get_from_sq(bb, data, side);
    break;
  default:
    break;
  }

  return std::nullopt;
}

}

int Board::see_move(const Move move) {
  perform_move(move);

  const auto us    = move_side(move);
  const auto them  = ~us;
  const auto score = !is_attacked(king_square[us], them)
                   ? see_rec(material_change(move), next_to_capture(move), move_to(move), them)
                   : SEE_INVALID_SCORE;

  unperform_move(move);
  return score;
}

int Board::see_last_move(const Move move) {
  return see_rec(material_change(move), next_to_capture(move), move_to(move), ~move_side(move));
}

int Board::see_rec(const int mat_change, const Piece next_capture, const Square to, const Color side_to_move) {

  SeeData data{{Pawn, Pawn}, {pieces(Pawn, WHITE), pieces(Pawn, BLACK)}};
  const auto rr = relative_rank(side_to_move, to);
  Move move;

  do
  {
    const auto from = lookup_best_attacker(data, to, side_to_move, this);
    if (!from)
      return mat_change;

    const auto current_pt = data.current_pt[side_to_move];

    move = current_pt == Pawn && rr == RANK_8
         ? init_move<PROMOTION | CAPTURE>(make_piece(current_pt, side_to_move), next_capture, from.value(), to, make_piece(Queen, side_to_move))
         : init_move<CAPTURE>(make_piece(current_pt, side_to_move), next_capture, from.value(), to, NoPiece);

    perform_move(move);

    if (!is_attacked(king_square[side_to_move], ~side_to_move))
      break;

    unperform_move(move);
  } while (true);

  const auto score = -see_rec(material_change(move), next_to_capture(move), move_to(move), ~move_side(move));

  unperform_move(move);

  return score < 0 ? mat_change + score : mat_change;
}
