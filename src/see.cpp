#include "board.h"
#include "material.h"
#include "bitboard.h"
#include "magic.h"
#include "move.h"

namespace {

constexpr int SEE_INVALID_SCORE = -5000;

constexpr int material_change(const uint32_t move) {
  return (is_capture(move) ? piece_value(move_captured(move)) : 0) + (is_promotion(move) ? (piece_value(move_promoted(move)) - piece_value(Pawn)) : 0);
}

constexpr int next_to_capture(const uint32_t move) {
  return is_promotion(move) ? move_promoted(move) : move_piece(move);
}

}

int Board::see_move(uint32_t move) {
  int score;
  make_move(move);

  if (!is_attacked(king_square[move_side(move)], ~move_side(move)))
  {
    init_see_move();
    score = see_rec(material_change(move), next_to_capture(move), move_to(move), ~move_side(move));
  } else
    score = SEE_INVALID_SCORE;

  unmake_move(move);
  return score;
}

int Board::see_last_move(uint32_t move) {
  init_see_move();
  return see_rec(material_change(move), next_to_capture(move), move_to(move), ~move_side(move));
}

int Board::see_rec(int mat_change, int next_capture, Square to, Color side_to_move) {
  Square from;
  uint32_t move;
  const auto rr = relative_rank(side_to_move, to);

  do
  {
    if (!lookup_best_attacker(to, side_to_move, from))
      return mat_change;

    if (current_piece[side_to_move] == Pawn && rr == RANK_8)
      init_move(move, current_piece[side_to_move] | (side_to_move << 3), next_capture, from, to, PROMOTION | CAPTURE, Queen | (side_to_move << 3));
    else
      init_move(move, current_piece[side_to_move] | (side_to_move << 3), next_capture, from, to, CAPTURE, 0);

    make_move(move);

    if (!is_attacked(king_square[side_to_move], ~side_to_move))
      break;

    unmake_move(move);
  } while (true);

  const auto score = -see_rec(material_change(move), next_to_capture(move), move_to(move), ~move_side(move));

  unmake_move(move);

  return score < 0 ? mat_change + score : mat_change;
}


bool Board::lookup_best_attacker(const Square to, const Color side, Square &from) {
  // "Best" == "Lowest piece value"

  switch (current_piece[side])
  {
  case Pawn:
    if (current_piece_bitboard[side] & pawn_captures[to | ((~side) << 6)])
    {
      from = lsb(current_piece_bitboard[side] & pawn_captures[to | ((~side) << 6)]);
      current_piece_bitboard[side] &= ~bit(from);
      return true;
    }
    current_piece[side]++;
    current_piece_bitboard[side] = knights(side);
    [[fallthrough]];
  case Knight:
    if (current_piece_bitboard[side] & knightAttacks(to))
    {
      from = lsb(current_piece_bitboard[side] & knightAttacks(to));
      current_piece_bitboard[side] &= ~bit(from);
      return true;
    }
    current_piece[side]++;
    current_piece_bitboard[side] = bishops(side);
    [[fallthrough]];

  case Bishop:
    if (current_piece_bitboard[side] & bishopAttacks(to, occupied))
    {
      from = lsb(current_piece_bitboard[side] & bishopAttacks(to, occupied));
      current_piece_bitboard[side] &= ~bit(from);
      return true;
    }
    current_piece[side]++;
    current_piece_bitboard[side] = rooks(side);
    [[fallthrough]];
  case Rook:
    if (current_piece_bitboard[side] & rookAttacks(to, occupied))
    {
      from = lsb(current_piece_bitboard[side] & rookAttacks(to, occupied));
      current_piece_bitboard[side] &= ~bit(from);
      return true;
    }
    current_piece[side]++;
    current_piece_bitboard[side] = queens(side);
    [[fallthrough]];
  case Queen:
    if (current_piece_bitboard[side] & queenAttacks(to, occupied))
    {
      from = lsb(current_piece_bitboard[side] & queenAttacks(to, occupied));
      current_piece_bitboard[side] &= ~bit(from);
      return true;
    }
    current_piece[side]++;
    current_piece_bitboard[side] = king(side);
    [[fallthrough]];
  case King:
    if (current_piece_bitboard[side] & kingAttacks(to))
    {
      from = lsb(current_piece_bitboard[side] & kingAttacks(to));
      current_piece_bitboard[side] &= ~bit(from);
      return true;
    }[[fallthrough]];
  default:
    return false;
  }
}

void Board::init_see_move() {
  current_piece.fill(Pawn);
  current_piece_bitboard[WHITE] = piece[Pawn];
  current_piece_bitboard[BLACK] = piece[Pawn | 8];
}
