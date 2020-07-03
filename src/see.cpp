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
  return (is_capture(move) ? piece_value(move_captured(move)) : 0) + (is_promotion(move) ? (piece_value(move_promoted(move)) - piece_value(PAWN)) : 0);
}

constexpr Piece next_to_capture(const Move move) {
  return is_promotion(move) ? move_promoted(move) : move_piece(move);
}

}

int Board::see_move(const Move m) {
  int score;
  perform_move(m);

  const auto us   = move_side(m);
  const auto them = ~us;

  if (!is_attacked(king_square[us], them))
  {
    init_see_move();
    score = see_rec(material_change(m), next_to_capture(m), move_to(m), them);
  } else
    score = SEE_INVALID_SCORE;

  unperform_move(m);
  return score;
}

int Board::see_last_move(const Move m) {
  init_see_move();
  return see_rec(material_change(m), next_to_capture(m), move_to(m), ~move_side(m));
}

int Board::see_rec(const int mat_change, const Piece next_capture, const Square to, const Color c) {
  const auto rr = relative_rank(c, to);

  Move move;

  do
  {
    const auto from = lookup_best_attacker(to, c);
    if (!from)
      return mat_change;

    const auto current_pt = current_piece[c];

    move = current_pt == PAWN && rr == RANK_8
         ? init_move<PROMOTION | CAPTURE>(make_piece(current_pt, c), next_capture, from.value(), to, make_piece(QUEEN, c))
         : init_move<CAPTURE>(make_piece(current_pt, c), next_capture, from.value(), to, NO_PIECE);

    perform_move(move);

    if (!is_attacked(king_square[c], ~c))
      break;

    unperform_move(move);
  } while (true);

  const auto score = -see_rec(material_change(move), next_to_capture(move), move_to(move), ~move_side(move));

  unperform_move(move);

  return score < 0 ? mat_change + score : mat_change;
}

std::optional<Square> Board::lookup_best_attacker(const Square to, const Color c) {
  // "Best" == "Lowest piece value"

  Bitboard b;

  const auto get_from_sq = [&](const Bitboard bb)
  {
    const auto from = lsb(bb);
    current_piece_bitboard[c] &= ~bit(from);
    return std::make_optional(from);
  };

  switch (current_piece[c])
  {
  case PAWN:
    b = current_piece_bitboard[c] & pawn_attacks_bb(~c, to);
    if (b)
      return get_from_sq(b);
    ++current_piece[c];
    current_piece_bitboard[c] = pieces(KNIGHT, c);
    [[fallthrough]];
  case KNIGHT:
    b = current_piece_bitboard[c] & piece_attacks_bb<KNIGHT>(to);
    if (b)
      return get_from_sq(b);
    ++current_piece[c];
    current_piece_bitboard[c] = pieces(BISHOP, c);
    [[fallthrough]];

  case BISHOP:
    b = current_piece_bitboard[c] & piece_attacks_bb<BISHOP>(to, occupied);
    if (b)
      return get_from_sq(b);
    ++current_piece[c];
    current_piece_bitboard[c] = pieces(ROOK, c);
    [[fallthrough]];
  case ROOK:
    b = current_piece_bitboard[c] & piece_attacks_bb<ROOK>(to, occupied);
    if (b)
      return get_from_sq(b);
    ++current_piece[c];
    current_piece_bitboard[c] = pieces(QUEEN, c);
    [[fallthrough]];
  case QUEEN:
    b = current_piece_bitboard[c] & piece_attacks_bb<QUEEN>(to, occupied);
    if (b)
      return get_from_sq(b);
    ++current_piece[c];
    current_piece_bitboard[c] = pieces(KING, c);
    [[fallthrough]];
  case KING:
    b = current_piece_bitboard[c] & piece_attacks_bb<KING>(to);
    if (b)
      return get_from_sq(b);
    break;
  default:
    break;
  }

  return std::nullopt;

}

void Board::init_see_move() {
  current_piece.fill(PAWN);
  current_piece_bitboard[WHITE] = pieces(PAWN, WHITE);
  current_piece_bitboard[BLACK] = pieces(PAWN, BLACK);
}
