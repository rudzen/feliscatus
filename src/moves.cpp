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

constexpr std::array<PieceType, 5> MoveGenPieceTypes{QUEEN, ROOK, BISHOP, KNIGHT, KING};

}// namespace

void Moves::generate_moves(MoveSorter *sorter, const Move tt_move, const int flags) {
  reset(sorter, tt_move, flags);
  max_stage_ = 3;

  if (move_flags_ & STAGES)
    return;

  const auto stm = b->side_to_move();

  generate_hash_move();
  if (stm == WHITE)
  {
    generate_captures_and_promotions<WHITE>();
    generate_quiet_moves<WHITE>();
  } else
  {
    generate_captures_and_promotions<BLACK>();
    generate_quiet_moves<BLACK>();
  }
}

void Moves::generate_captures_and_promotions(MoveSorter *sorter) {
  reset(sorter, MOVE_NONE, STAGES);
  max_stage_ = 2;
  stage_     = 1;
}

template<Color Us>
void Moves::generate_moves(const PieceType pt, const Bitboard to_squares) {
  reset(nullptr, MOVE_NONE, 0);

  const auto pieces = b->pieces();
  auto bb           = b->pieces(pt, Us);

  while (bb)
  {
    const auto from = pop_lsb(&bb);
    add_moves<Us>(pt, from, piece_attacks_bb(pt, from, pieces) & to_squares);
  }
}

template void Moves::generate_moves<WHITE>(PieceType, Bitboard);
template void Moves::generate_moves<BLACK>(PieceType, Bitboard);

void Moves::generate_pawn_moves(const bool capture, const Bitboard to_squares, const Color c) {
  reset(nullptr, MOVE_NONE, 0);

  if (c == WHITE)
  {
    if (capture)
      add_pawn_capture_moves<WHITE>(to_squares);
    else
      add_pawn_quiet_moves<WHITE>(to_squares);
  } else
  {
    if (capture)
      add_pawn_capture_moves<BLACK>(to_squares);
    else
      add_pawn_quiet_moves<BLACK>(to_squares);
  }
}

MoveData *Moves::next_move() {
  return b->side_to_move() == WHITE ? get_next_move<WHITE>() : get_next_move<BLACK>();
}

bool Moves::is_pseudo_legal(const Move m) const {
  // TODO : castleling & en passant moves

  assert(is_ok(m));

  const auto from = move_from(m);
  const auto pc   = move_piece(m);

  if ((b->pieces(pc) & from) == 0)
    return false;

  const auto to = move_to(m);

  if (const auto move_stm = move_side(m); move_stm != b->side_to_move())
    return false;
  else if (is_capture(m))
  {
    if ((b->pieces(~move_stm) & to) == 0)
      return false;
    if ((b->pieces(move_captured(m)) & to) == 0)
      return false;
  }
  // } else if (is_castle_move(m))
  //    return !b->is_attacked(b->king_sq(side_to_move), side_to_move) && !in_check && ((from < to && can_castle_short()) || (from > to && can_castle_long()));
  else if (b->pieces() & to)
    return false;

  if (const auto pt = type_of(move_piece(m)); util::in_between<QUEEN, BISHOP>(pt))
    if (between_bb[from][to] & b->pieces())
      return false;

  return true;
}

void Moves::reset(MoveSorter *sorter, const Move m, const int flags) {
  move_sorter_ = sorter;
  transp_move_ = m;
  move_flags_  = flags;
  iteration_   = number_moves_ = stage_ = 0;

  [[likely]]
  if (m)
  {
    const auto mt = type_of(m);
    [[unlikely]]
    if (mt & (CASTLE | EPCAPTURE))
    {
      // needed because is_pseudo_legal() is not complete yet.
      transp_move_ = MOVE_NONE;
      move_flags_ &= ~STAGES;
    }
  }

  if (flags & LEGALMOVES)
  {
    const auto stm = b->side_to_move();
    b->pinned(b->get_pinned_pieces(stm, b->king_sq(stm)));
  }
}

void Moves::generate_hash_move() {
  if (transp_move_ && is_pseudo_legal(transp_move_))
  {
    move_list[number_moves_].score  = 890010;
    move_list[number_moves_++].move = transp_move_;
  }
  stage_++;
}

template<Color Us>
void Moves::generate_captures_and_promotions() {
  constexpr auto Them         = ~Us;
  constexpr auto WestAttacks  = pawn_west_attacks[Us];
  constexpr auto EastAttacks  = pawn_east_attacks[Us];
  constexpr auto WestDistance = pawn_west_attack_dist[Us];
  constexpr auto EastDistance = pawn_east_attack_dist[Us];
  constexpr auto Rank_7       = rank_7[Us];
  constexpr auto Up           = pawn_push(Us);
  const auto opponent_pieces  = b->pieces(Them);
  const auto pawns            = b->pieces(PAWN, Us);

  add_moves<Us>(opponent_pieces);
  add_pawn_moves<Us>(pawn_push(Us, pawns & Rank_7) & ~b->pieces(), Up, NORMAL);
  add_pawn_moves<Us>(WestAttacks(pawns) & opponent_pieces, WestDistance, CAPTURE);
  add_pawn_moves<Us>(EastAttacks(pawns) & opponent_pieces, EastDistance, CAPTURE);
  [[unlikely]]
  if (const auto en_passant_square = b->en_passant_square(); en_passant_square != NO_SQ)
  {
    add_pawn_moves<Us>(WestAttacks(pawns) & en_passant_square, WestDistance, EPCAPTURE);
    add_pawn_moves<Us>(EastAttacks(pawns) & en_passant_square, EastDistance, EPCAPTURE);
  }
  stage_++;
}

template<Color Us>
MoveData *Moves::get_next_move() {
  while (iteration_ == number_moves_ && stage_ < max_stage_)
  {
    switch (stage_)
    {
    case 0:
      generate_hash_move();
      break;

    case 1:
      generate_captures_and_promotions<Us>();
      break;

    case 2:
      generate_quiet_moves<Us>();
      break;

    default:// error
      return nullptr;
    }
  }

  if (iteration_ == number_moves_)
    return nullptr;

  if (!move_sorter_)
    return &move_list[iteration_++];

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
      generate_quiet_moves<Us>();
      continue;
    }
    std::swap(move_list[iteration_], move_list[best_idx]);
    return &move_list[iteration_++];
  } while (true);
}

template<Color Us>
void Moves::generate_quiet_moves() {
  constexpr auto NotRank7  = ~rank_7[Us];
  constexpr auto Rank3     = rank_3[Us];
  constexpr auto Up        = pawn_push(Us);
  const auto empty_squares = ~b->pieces();
  const auto pushed        = shift_bb<Up>(b->pieces(PAWN, Us) & NotRank7) & empty_squares;

  if (!b->in_check())
  {
    if (can_castle_short<Us>())
      add_castle_move<Us>(oo_king_from[Us], oo_king_to[Us]);

    if (can_castle_long<Us>())
      add_castle_move<Us>(ooo_king_from[Us], ooo_king_to[Us]);
  }

  add_pawn_moves<Us>(pushed, Up, NORMAL);
  add_pawn_moves<Us>(shift_bb<Up>(pushed & Rank3) & empty_squares, Up * 2, DOUBLEPUSH);
  add_moves<Us>(empty_squares);
  stage_++;
}

template<Color Us>
void Moves::add_move(const Piece pc, const Square from, const Square to, const MoveType mt, const Piece promoted) {
  constexpr auto Them = ~Us;

  const auto get_captured = [&]()
  {
    if (mt & CAPTURE)
      return b->get_piece(to);
    if (mt & EPCAPTURE)
      return make_piece(PAWN, Them);
    return NO_PIECE;
  };

  const auto captured = get_captured();
  const auto move     = init_move(pc, captured, from, to, mt, promoted);

  if (transp_move_ == move)
    return;

  [[unlikely]]
  if (move_flags_ & LEGALMOVES && !b->is_legal(move, pc, from, mt))
    return;

  auto &move_data = move_list[number_moves_++];
  move_data       = move;

  [[likely]]
  if (move_sorter_)
    move_sorter_->sort_move(move_data);
  else
    move_data.score = 0;
}

template<Color Us, PieceType Pt>
void Moves::add_piece_moves(const Bitboard to_squares) {
  const auto pieces = b->pieces();

  auto bb = b->pieces(Pt, Us);
  while (bb)
  {
    const auto from = pop_lsb(&bb);
    add_moves<Us>(Pt, from, piece_attacks_bb<Pt>(from, pieces) & to_squares);
  }
}

template<Color Us>
void Moves::add_moves(const Bitboard to_squares) {
  add_piece_moves<Us, KING  >(to_squares);
  add_piece_moves<Us, QUEEN >(to_squares);
  add_piece_moves<Us, ROOK  >(to_squares);
  add_piece_moves<Us, BISHOP>(to_squares);
  add_piece_moves<Us, KNIGHT>(to_squares);
}

template<Color Us>
void Moves::add_moves(const PieceType pt, const Square from, const Bitboard attacks) {
  const auto pc = make_piece(pt, Us);

  for (auto bb = attacks; bb;)
  {
    const auto to = pop_lsb(&bb);
    add_move<Us>(pc, from, to, b->get_piece(to) == NO_PIECE ? NORMAL : CAPTURE);
  }
}

template<Color Us>
void Moves::add_pawn_quiet_moves(const Bitboard to_squares) {
  constexpr auto Rank_3    = relative_rank(Us, RANK_3);
  const auto empty_squares = ~b->pieces();
  const auto pushed        = pawn_push(Us, b->pieces(PAWN, Us)) & empty_squares;

  add_pawn_moves<Us>(pushed & to_squares, pawn_push(Us), NORMAL);
  add_pawn_moves<Us>(pawn_push(Us, pushed & Rank_3) & empty_squares & to_squares, pawn_push(Us) * 2, DOUBLEPUSH);
}

template<Color Us>
void Moves::add_pawn_capture_moves(const Bitboard to_squares) {
  constexpr auto Them         = ~Us;
  constexpr auto WestAttacks  = pawn_west_attacks[Us];
  constexpr auto EastAttacks  = pawn_east_attacks[Us];
  constexpr auto WestDistance = pawn_west_attack_dist[Us];
  constexpr auto EastDistance = pawn_east_attack_dist[Us];
  const auto opponent_pieces  = b->pieces(Them);
  const auto pawns            = b->pieces(PAWN, Us);

  add_pawn_moves<Us>(WestAttacks(pawns) & opponent_pieces & to_squares, WestDistance, CAPTURE);
  add_pawn_moves<Us>(EastAttacks(pawns) & opponent_pieces & to_squares, EastDistance, CAPTURE);
  [[unlikely]]
  if (b->en_passant_square() != NO_SQ)
  {
    add_pawn_moves<Us>(WestAttacks(pawns) & to_squares & b->en_passant_square(), WestDistance, EPCAPTURE);
    add_pawn_moves<Us>(EastAttacks(pawns) & to_squares & b->en_passant_square(), EastDistance, EPCAPTURE);
  }
}

template<Color Us>
void Moves::add_pawn_moves(const Bitboard to_squares, const Direction d, const MoveType mt) {
  constexpr auto Rank8 = bit(relative_rank(Us, RANK_8));
  const auto pawn = make_piece(PAWN, Us);

  // promotion moves

  auto targets = to_squares & Rank8;

  while (targets)
  {
    const auto to   = pop_lsb(&targets);
    const auto from = to - d;
    const auto promo_type = mt | PROMOTION;
    for (const auto promoted : PromotionPieceTypes)
      add_move<Us>(pawn, from, to, promo_type, make_piece(promoted, Us));
  }

  // non-promotion moves

  targets = to_squares & ~Rank8;

  while (targets)
  {
    const auto to   = pop_lsb(&targets);
    add_move<Us>(pawn, to - d, to, mt);
  }
}

template<Color Us>
void Moves::add_castle_move(const Square from, const Square to) {
  add_move<Us>(make_piece(KING, Us), from, to, CASTLE);
}

template<Color Us>
bool Moves::can_castle_short() const {
  return b->castle_rights() & oo_allowed_mask[Us] && b->is_castleling_impeeded(oo_king_to[Us], Us);
}

template<Color Us>
bool Moves::can_castle_long() const {
  return b->castle_rights() & ooo_allowed_mask[Us] && b->is_castleling_impeeded(ooo_king_to[Us], Us);
}
