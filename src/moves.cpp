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

#include <ranges>

#include "moves.h"
#include "board.h"
#include "bitboard.h"
#include "magic.h"

namespace {

constexpr std::array<PieceType, 5> MoveGenPieceTypes { Queen, Rook, Bishop, Knight, King };

[[nodiscard]]
bool is_castle_allowed(const Square to, const Color stm, const Board* b) {
  // A bit complicated because of Chess960. See http://en.wikipedia.org/wiki/Chess960
  // The following comments were taken from that source.

  // Check that the smallest back rank interval containing the king, the castling rook, and their
  // destination squares, contains no pieces other than the king and castling rook.

  const auto rook_to          = rook_castles_to[to];
  const auto rook_from        = rook_castles_from[to];
  const auto king_square      = b->king_sq(stm);
  const auto bb_castle_pieces = bit(rook_from, king_square);

  if (const auto bb_castle_span = bb_castle_pieces | between_bb[king_square][rook_from] | between_bb[rook_from][rook_to] | bit(rook_to, to);
    (bb_castle_span & b->pieces()) != bb_castle_pieces)
    return false;

  // Check that no square between the king's initial and final squares (including the initial and final
  // squares) may be under attack by an enemy piece. (Initial square was already checked a this point.)

  for (auto bb = between_bb[king_square][to] | to; bb; reset_lsb(bb))
    if (b->is_attacked(lsb(bb), ~stm))
      return false;

  return true;
}

}

void Moves::generate_moves(MoveSorter *sorter, const Move tt_move, const int flags) {
  reset(sorter, tt_move, flags);
  max_stage_ = 3;

  if (move_flags_ & STAGES)
    return;

  generate_hash_move();
  generate_captures_and_promotions();
  generate_quiet_moves();
}

void Moves::generate_captures_and_promotions(MoveSorter *sorter) {
  reset(sorter, MOVE_NONE, QUEENPROMOTION | STAGES);
  max_stage_ = 2;
  stage_     = 1;
}

void Moves::generate_moves(const PieceType pt, const Bitboard to_squares) {
  reset(nullptr, MOVE_NONE, 0);

  const auto pieces = b->pieces();
  auto bb = b->pieces(pt, side_to_move);

  while (bb)
  {
    const auto from = pop_lsb(&bb);
    add_moves(pt, from, piece_attacks_bb(pt, from, pieces) & to_squares);
  }
}

void Moves::generate_pawn_moves(const bool capture, const Bitboard to_squares) {
  reset(nullptr, MOVE_NONE, 0);

  if (capture)
    add_pawn_capture_moves(to_squares);
  else
    add_pawn_quiet_moves(to_squares);
}

MoveData *Moves::next_move() {
  while (iteration_ == number_moves_ && stage_ < max_stage_)
  {
    switch (stage_)
    {
    case 0:
      generate_hash_move();
      break;

    case 1:
      generate_captures_and_promotions();
      break;

    case 2:
      generate_quiet_moves();
      break;

    default: // error
      return nullptr;
    }
  }

  if (iteration_ == number_moves_)
    return nullptr;

  do
  {
    auto best_idx   = iteration_;
    auto best_score = move_list[best_idx].score;

    for (auto i = best_idx + 1; i < number_moves_; ++i)
    {
      if (move_list[i].score > best_score)
      {
        best_score = move_list[i].score;
        best_idx   = i;
      }
    }

    if (max_stage_ > 2 && stage_ == 2 && move_list[best_idx].score < 0)
    {
      generate_quiet_moves();
      continue;
    }
    std::swap(move_list[iteration_], move_list[best_idx]);
    return &move_list[iteration_++];
  }
  while (true);
}

bool Moves::is_pseudo_legal(const Move m) const {
  // TODO : castleling & en passant moves

  const auto from = move_from(m);

  if ((b->piece[move_piece(m)] & from) == 0)
    return false;

  const auto to = move_to(m);

  if (const auto move_stm = move_side(m); move_stm != side_to_move)
    return false;
  else if (is_capture(m))
  {
    if ((b->pieces(~move_stm) & to) == 0)
      return false;
    if ((b->piece[move_captured(m)] & to) == 0)
      return false;
  }
  // } else if (is_castle_move(m))
  //    return !b->is_attacked(b->king_sq(side_to_move), side_to_move) && !in_check && ((from < to && can_castle_short()) || (from > to && can_castle_long()));
  else if (b->pieces() & to)
     return false;

  if (const auto pt = type_of(move_piece(m)); util::in_between<Bishop, Queen>(pt))
    if (between_bb[from][to] & b->pieces())
      return false;

  return true;
}

void Moves::reset(MoveSorter *sorter, const Move move, const int flags) {
  move_sorter_ = sorter;
  transp_move_ = move;
  move_flags_  = flags;
  iteration_   = number_moves_ = stage_ = 0;

  if (move)
  {
    if (is_castle_move(move) || is_ep_capture(move))
    {
      // needed because is_pseudo_legal() is not complete yet.
      transp_move_ = MOVE_NONE;
      move_flags_ &= ~STAGES;
    }
  }

  if (flags & LEGALMOVES)
    pinned_ = b->get_pinned_pieces(side_to_move, b->king_sq(side_to_move));
}

void Moves::generate_hash_move() {
  if (transp_move_ && is_pseudo_legal(transp_move_))
  {
    move_list[number_moves_].score  = 890010;
    move_list[number_moves_++].move = transp_move_;
  }
  stage_++;
}

void Moves::generate_captures_and_promotions() {
  const auto opponent_pieces = b->pieces(~side_to_move);
  const auto pawns           = b->pieces(Pawn, side_to_move);

  add_moves(opponent_pieces);
  add_pawn_moves(pawn_push(side_to_move, pawns & rank_7[side_to_move]) & ~b->pieces(), pawn_push(side_to_move), NORMAL);
  add_pawn_moves(pawn_west_attacks[side_to_move](pawns) & opponent_pieces, pawn_west_attack_dist[side_to_move], CAPTURE);
  add_pawn_moves(pawn_east_attacks[side_to_move](pawns) & opponent_pieces, pawn_east_attack_dist[side_to_move], CAPTURE);
  if (en_passant_square != no_square)
  {
    add_pawn_moves(pawn_west_attacks[side_to_move](pawns) & en_passant_square, pawn_west_attack_dist[side_to_move], EPCAPTURE);
    add_pawn_moves(pawn_east_attacks[side_to_move](pawns) & en_passant_square, pawn_east_attack_dist[side_to_move], EPCAPTURE);
  }
  stage_++;
}

void Moves::generate_quiet_moves() {
  if (!in_check)
  {
    if (can_castle_short())
      add_castle_move(oo_king_from[side_to_move], oo_king_to[side_to_move]);

    if (can_castle_long())
      add_castle_move(ooo_king_from[side_to_move], ooo_king_to[side_to_move]);
  }
  const auto empty_squares = ~b->pieces();
  const auto pushed        = pawn_push(side_to_move, b->pieces(Pawn, side_to_move) & ~rank_7[side_to_move]) & empty_squares;
  add_pawn_moves(pushed, pawn_push(side_to_move), NORMAL);
  add_pawn_moves(pawn_push(side_to_move, pushed & rank_3[side_to_move]) & empty_squares, pawn_push(side_to_move) * 2, DOUBLEPUSH);
  add_moves(empty_squares);
  stage_++;
}

void Moves::add_move(const Piece piece, const Square from, const Square to, const MoveType type, const Piece promoted) {
  Piece captured;

  if (type & CAPTURE)
    captured = b->get_piece(to);
  else if (type & EPCAPTURE)
    captured = make_piece(Pawn, ~side_to_move);
  else
    captured = NoPiece;

  const auto move = init_move(piece, captured, from, to, type, promoted);

  if (transp_move_ == move)
    return;

  if (move_flags_ & LEGALMOVES && !is_legal(move, piece, from, type))
    return;

  auto &move_data = move_list[number_moves_++];
  move_data  = move;

  if (move_sorter_)
    move_sorter_->sort_move(move_data);
  else
    move_data.score = 0;
}

void Moves::add_moves(const Bitboard to_squares) {
  const auto pieces = b->pieces();
  for (const auto pt : MoveGenPieceTypes)
  {
    auto bb = b->pieces(pt, side_to_move);
    while (bb)
    {
      const auto from = pop_lsb(&bb);
      add_moves(pt, from, piece_attacks_bb(pt, from, pieces) & to_squares);
    }
  }
}

void Moves::add_moves(const PieceType pt, const Square from, const Bitboard attacks) {
  const auto pc = make_piece(pt, side_to_move);
  for (auto bb = attacks; bb;)
  {
    const auto to = pop_lsb(&bb);
    add_move(pc, from, to, b->get_piece(to) == NoPiece ? NORMAL : CAPTURE);
  }
}

void Moves::add_pawn_quiet_moves(const Bitboard to_squares) {
  const auto empty_squares = ~b->pieces();
  const auto pushed        = pawn_push(side_to_move, b->pieces(Pawn, side_to_move)) & empty_squares;
  add_pawn_moves(pushed & to_squares, pawn_push(side_to_move), NORMAL);
  add_pawn_moves(pawn_push(side_to_move, pushed & rank_3[side_to_move]) & empty_squares & to_squares, pawn_push(side_to_move) * 2, DOUBLEPUSH);
}

void Moves::add_pawn_capture_moves(const Bitboard to_squares) {
  const auto opponent_pieces = b->pieces(~side_to_move);
  const auto pawns = b->pieces(Pawn, side_to_move);
  add_pawn_moves(pawn_west_attacks[side_to_move](pawns) & opponent_pieces & to_squares, pawn_west_attack_dist[side_to_move], CAPTURE);
  add_pawn_moves(pawn_east_attacks[side_to_move](pawns) & opponent_pieces & to_squares, pawn_east_attack_dist[side_to_move], CAPTURE);
  if (en_passant_square != no_square)
  {
    add_pawn_moves(pawn_west_attacks[side_to_move](pawns) & to_squares & en_passant_square, pawn_west_attack_dist[side_to_move], EPCAPTURE);
    add_pawn_moves(pawn_east_attacks[side_to_move](pawns) & to_squares & en_passant_square, pawn_east_attack_dist[side_to_move], EPCAPTURE);
  }
}

void Moves::add_pawn_moves(const Bitboard to_squares, const Direction distance, const MoveType type) {

  const auto pawn = make_piece(Pawn, side_to_move);

  for (auto bb = to_squares; bb; reset_lsb(bb))
  {
    const auto to   = lsb(bb);
    const auto from = to - distance;

    if (const auto rr = relative_rank(side_to_move, to); rr == RANK_8)
    {
      const auto promo_type = type | PROMOTION;

      if (move_flags_ & QUEENPROMOTION)
      {
        add_move(pawn, from, to, promo_type, make_piece(Queen, side_to_move));
        return;
      }

      for (const auto promoted : PromotionPieceTypes)
        add_move(pawn, from, to, promo_type, make_piece(promoted, side_to_move));
    } else
      add_move(pawn, from, to, type);
  }
}

void Moves::add_castle_move(const Square from, const Square to) { add_move(make_piece(King, side_to_move), from, to, CASTLE); }

bool Moves::gives_check(const Move m) const {
  b->perform_move(m);
  const auto is_attacked = b->is_attacked(b->king_sq(~side_to_move), side_to_move);
  b->unperform_move(m);
  return is_attacked;
}

bool Moves::is_legal(const Move m, const Piece piece, const Square from, const MoveType type) const {
  if (!(pinned_ & from) && !in_check && type_of(piece) != King && !(type & EPCAPTURE))
    return true;

  b->perform_move(m);

  const auto is_attacked = b->is_attacked(b->king_sq(side_to_move), ~side_to_move);
  b->unperform_move(m);
  return !is_attacked;
}

bool Moves::can_castle_short() const { return castle_rights & oo_allowed_mask[side_to_move] && is_castle_allowed(oo_king_to[side_to_move], side_to_move, b); }

bool Moves::can_castle_long() const { return castle_rights & ooo_allowed_mask[side_to_move] && is_castle_allowed(ooo_king_to[side_to_move], side_to_move, b); }
