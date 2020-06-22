#include <ranges>
#include "moves.h"
#include "board.h"
#include "bitboard.h"
#include "magic.h"


namespace {

[[nodiscard]]
bool is_castle_allowed(const Square to, const Color stm, const Board* b) {
  // A bit complicated because of Chess960. See http://en.wikipedia.org/wiki/Chess960
  // The following comments were taken from that source.

  // Check that the smallest back rank interval containing the king, the castling rook, and their
  // destination squares, contains no pieces other than the king and castling rook.

  const auto rook_to          = rook_castles_to[to];
  const auto rook_from        = rook_castles_from[to];
  const auto king_square      = b->king_square[stm];
  const auto bb_castle_pieces = bit(rook_from, king_square);

  if (const auto bb_castle_span = bb_castle_pieces | between_bb[king_square][rook_from] | between_bb[rook_from][rook_to] | bit(rook_to, to);
    (bb_castle_span & b->occupied) != bb_castle_pieces)
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
  max_stage = 3;

  if ((move_flags & STAGES) != 0)
    return;

  generate_hash_move();
  generate_captures_and_promotions();
  generate_quiet_moves();
}

void Moves::generate_captures_and_promotions(MoveSorter *sorter) {
  reset(sorter, MOVE_NONE, QUEENPROMOTION | STAGES);
  max_stage = 2;
  stage     = 1;
}

void Moves::generate_moves(const int piece, const Bitboard to_squares) {
  reset(nullptr, MOVE_NONE, 0);

  for (auto bb = b->piece[piece]; bb; reset_lsb(bb))
  {
    const auto from = lsb(bb);
    add_moves(piece, from, piece_attacks_bb(piece, from, b->occupied) & to_squares);
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
  while (iteration == number_moves && stage < max_stage)
  {
    switch (stage)
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

  if (iteration == number_moves)
    return nullptr;

  do
  {
    auto best_idx   = iteration;
    auto best_score = move_list[best_idx].score;

    for (auto i = best_idx + 1; i < number_moves; i++)
    {
      if (move_list[i].score > best_score)
      {
        best_score = move_list[i].score;
        best_idx   = i;
      }
    }

    if (max_stage > 2 && stage == 2 && move_list[best_idx].score < 0)
    {
      generate_quiet_moves();
      continue;
    }
    std::swap(move_list[iteration], move_list[best_idx]);
    return &move_list[iteration++];
  }
  while (true);
}

bool Moves::is_pseudo_legal(const Move m) const {
  // TODO : en passant moves and castle moves

  const auto from = move_from(m);

  if ((b->piece[move_piece(m)] & from) == 0)
    return false;

  const auto to = move_to(m);

  if (const auto move_stm = move_side(m); move_stm != side_to_move)
    return false;
  else if (is_capture(m))
  {
    if ((b->occupied_by_side[~move_stm] & to) == 0)
      return false;

    if ((b->piece[move_captured(m)] & to) == 0)
      return false;
  //} else if (is_castle_move(m))
  //{
  //  // TODO : finish
  //} else if (is_ep_capture(m))
  //{
  //  // TODO : finish
  } else if (b->occupied & to)
    return false;

  if (const auto piece = move_piece(m) & 7; piece == Bishop || piece == Rook || piece == Queen)
  {
    if (between_bb[from][to] & b->occupied)
      return false;
  }

  return true;
}

void Moves::reset(MoveSorter *sorter, const Move move, const int flags) {
  move_sorter = sorter;
  transp_move = move;
  move_flags  = flags;

  if (move)
  {
    if (is_castle_move(this->transp_move) || is_ep_capture(this->transp_move))
    {
      // needed because isPseudoLegal() is not complete yet.
      transp_move = MOVE_NONE;
      move_flags &= ~STAGES;
    }
  }

  iteration    = 0;
  number_moves = 0;
  stage        = 0;

  if (flags & LEGALMOVES)
    pinned = b->get_pinned_pieces(side_to_move, b->king_square[side_to_move]);
}

void Moves::generate_hash_move() {
  if (transp_move && is_pseudo_legal(transp_move))
  {
    move_list[number_moves].score  = 890010;
    move_list[number_moves++].move = transp_move;
  }
  stage++;
}

void Moves::generate_captures_and_promotions() {
  const auto opponent_pieces = b->occupied_by_side[~side_to_move];
  const auto pawns           = b->pawns(side_to_move);

  add_moves(opponent_pieces);
  add_pawn_moves(pawn_push(side_to_move, pawns & rank_7[side_to_move]) & ~b->occupied, pawn_push(side_to_move), NORMAL);
  add_pawn_moves(pawn_west_attacks[side_to_move](pawns) & opponent_pieces, pawn_west_attack_dist[side_to_move], CAPTURE);
  add_pawn_moves(pawn_east_attacks[side_to_move](pawns) & opponent_pieces, pawn_east_attack_dist[side_to_move], CAPTURE);
  if (en_passant_square != no_square)
  {
    add_pawn_moves(pawn_west_attacks[side_to_move](pawns) & en_passant_square, pawn_west_attack_dist[side_to_move], EPCAPTURE);
    add_pawn_moves(pawn_east_attacks[side_to_move](pawns) & en_passant_square, pawn_east_attack_dist[side_to_move], EPCAPTURE);
  }
  stage++;
}

void Moves::generate_quiet_moves() {
  if (!in_check)
  {
    if (can_castle_short())
      add_castle_move(oo_king_from[side_to_move], oo_king_to[side_to_move]);

    if (can_castle_long())
      add_castle_move(ooo_king_from[side_to_move], ooo_king_to[side_to_move]);
  }
  const auto empty_squares = ~b->occupied;
  const auto pushed        = pawn_push(side_to_move, b->pawns(side_to_move) & ~rank_7[side_to_move]) & empty_squares;
  add_pawn_moves(pushed, pawn_push(side_to_move), NORMAL);
  add_pawn_moves(pawn_push(side_to_move, pushed & rank_3[side_to_move]) & empty_squares, pawn_push(side_to_move) * 2, DOUBLEPUSH);
  add_moves(~b->occupied);
  stage++;
}

void Moves::add_move(const int piece, const Square from, const Square to, const MoveType type, const int promoted) {
  int captured;

  if (type & CAPTURE)
    captured = b->get_piece(to);
  else if (type & EPCAPTURE)
    captured = Pawn | ((~side_to_move) << 3);
  else
    captured = 0;

  const auto move = init_move(piece, captured, from, to, type, promoted);

  if (transp_move == move)
    return;

  if ((move_flags & LEGALMOVES) && !is_legal(move, piece, from, type))
    return;

  auto &move_data = move_list[number_moves++];
  move_data  = move;

  if (move_sorter)
    move_sorter->sort_move(move_data);
  else
    move_data.score = 0;
}

void Moves::add_moves(const Bitboard to_squares) {
  Bitboard bb;
  Square from;
  const auto mask = side_to_move << 3;

  auto pc = Queen | mask;

  for (bb = b->piece[pc]; bb; reset_lsb(bb))
  {
    from = lsb(bb);
    add_moves(pc, from, piece_attacks_bb<Queen>(from, b->occupied) & to_squares);
  }

  pc = Rook | mask;

  for (bb = b->piece[pc]; bb; reset_lsb(bb))
  {
    from = lsb(bb);
    add_moves(pc, from, piece_attacks_bb<Rook>(from, b->occupied) & to_squares);
  }

  pc = Bishop | mask;

  for (bb = b->piece[pc]; bb; reset_lsb(bb))
  {
    from = lsb(bb);
    add_moves(pc, from, piece_attacks_bb<Bishop>(from, b->occupied) & to_squares);
  }

  pc = Knight | mask;

  for (bb = b->piece[pc]; bb; reset_lsb(bb))
  {
    from = lsb(bb);
    add_moves(pc, from,  piece_attacks_bb<Knight>(from) & to_squares);
  }

  pc = King | mask;

  for (bb = b->piece[pc]; bb; reset_lsb(bb))
  {
    from = lsb(bb);
    add_moves(pc, from, piece_attacks_bb<King>(from) & to_squares);
  }
}

void Moves::add_moves(const int piece, const Square from, const Bitboard attacks) {
  const auto pc = piece | (side_to_move << 3);
  for (auto bb = attacks; bb; reset_lsb(bb))
  {
    const auto to = lsb(bb);
    add_move(pc, from, to, b->get_piece(to) == NoPiece ? NORMAL : CAPTURE);
  }
}

void Moves::add_pawn_quiet_moves(const Bitboard to_squares) {
  const auto empty_squares = ~b->occupied;
  const auto pushed        = pawn_push(side_to_move, b->pawns(side_to_move)) & empty_squares;
  add_pawn_moves(pushed & to_squares, pawn_push(side_to_move), NORMAL);
  add_pawn_moves(pawn_push(side_to_move, pushed & rank_3[side_to_move]) & empty_squares & to_squares, pawn_push(side_to_move) * 2, DOUBLEPUSH);
}

void Moves::add_pawn_capture_moves(const Bitboard to_squares) {
  const auto opponent_pieces = b->occupied_by_side[~side_to_move];
  const auto pawns = b->pawns(side_to_move);
  add_pawn_moves(pawn_west_attacks[side_to_move](pawns) & opponent_pieces & to_squares, pawn_west_attack_dist[side_to_move], CAPTURE);
  add_pawn_moves(pawn_east_attacks[side_to_move](pawns) & opponent_pieces & to_squares, pawn_east_attack_dist[side_to_move], CAPTURE);
  if (en_passant_square != no_square)
  {
    add_pawn_moves(pawn_west_attacks[side_to_move](pawns) & to_squares & en_passant_square, pawn_west_attack_dist[side_to_move], EPCAPTURE);
    add_pawn_moves(pawn_east_attacks[side_to_move](pawns) & to_squares & en_passant_square, pawn_east_attack_dist[side_to_move], EPCAPTURE);
  }
}

void Moves::add_pawn_moves(const Bitboard to_squares, const Direction distance, const MoveType type) {

  const auto mask = side_to_move << 3;
  const auto pawn = Pawn | mask;

  for (auto bb = to_squares; bb; reset_lsb(bb))
  {
    const auto to   = lsb(bb);
    const auto from = to - distance;

    if (const auto rr = relative_rank(side_to_move, to); rr == RANK_8)
    {
      const auto promo_type = type | PROMOTION;

      if (move_flags & QUEENPROMOTION)
      {
        add_move(pawn, from, to, promo_type, Queen | mask);
        return;
      }

      for (auto promoted = Queen; promoted >= Knight; promoted--)
        add_move(pawn, from, to, promo_type, promoted | mask);
    } else
      add_move(pawn, from, to, type);
  }
}

void Moves::add_castle_move(const Square from, const Square to) { add_move(King | (side_to_move << 3), from, to, CASTLE); }

bool Moves::gives_check(const Move m) const {
  b->make_move(m);
  const auto is_attacked = b->is_attacked(b->king_square[~side_to_move], side_to_move);
  b->unmake_move(m);
  return is_attacked;
}

bool Moves::is_legal(const Move m, const int piece, const Square from, const MoveType type) const {
  if (!(pinned & from) && !in_check && (piece & 7) != King && !(type & EPCAPTURE))
    return true;

  b->make_move(m);

  if (b->is_attacked(b->king_square[side_to_move], ~side_to_move))
  {
    b->unmake_move(m);
    return false;
  }
  b->unmake_move(m);
  return true;
}

bool Moves::can_castle_short() const { return castle_rights & oo_allowed_mask[side_to_move] && is_castle_allowed(oo_king_to[side_to_move], side_to_move, b); }

bool Moves::can_castle_long() const { return castle_rights & ooo_allowed_mask[side_to_move] && is_castle_allowed(ooo_king_to[side_to_move], side_to_move, b); }
