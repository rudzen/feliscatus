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

int Board::see_last_move(const uint32_t move) {
  init_see_move();
  return see_rec(material_change(move), next_to_capture(move), move_to(move), ~move_side(move));
}

int Board::see_rec(const int mat_change, const int next_capture, const Square to, const Color side_to_move) {
  const auto rr = relative_rank(side_to_move, to);

  uint32_t move;

  do
  {
    const auto from = lookup_best_attacker(to, side_to_move);
    if (!from.has_value())
      return mat_change;

    move = current_piece[side_to_move] == Pawn && rr == RANK_8
         ? init_move<PROMOTION | CAPTURE>(current_piece[side_to_move] | (side_to_move << 3), next_capture, from.value(), to, Queen | (side_to_move << 3))
         : init_move<CAPTURE>(current_piece[side_to_move] | (side_to_move << 3), next_capture, from.value(), to, 0);

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

  switch (current_piece[side])
  {
  case Pawn:
    b = current_piece_bitboard[side] & pawn_captures[to | ((~side) << 6)];
    if (b)
    {
      const auto from = lsb(b);
      current_piece_bitboard[side] &= ~bit(from);
      return std::optional<Square>(from);
    }
    current_piece[side]++;
    current_piece_bitboard[side] = knights(side);
    [[fallthrough]];
  case Knight:
    b = current_piece_bitboard[side] & piece_attacks_bb<Knight>(to);
    if (b)
    {
      const auto from = lsb(b);
      current_piece_bitboard[side] &= ~bit(from);
      return std::optional<Square>(from);
    }
    current_piece[side]++;
    current_piece_bitboard[side] = bishops(side);
    [[fallthrough]];

  case Bishop:
    b = current_piece_bitboard[side] & piece_attacks_bb<Bishop>(to, occupied);
    if (b)
    {
      const auto from = lsb(b);
      current_piece_bitboard[side] &= ~bit(from);
      return std::optional<Square>(from);
    }
    current_piece[side]++;
    current_piece_bitboard[side] = rooks(side);
    [[fallthrough]];
  case Rook:
    b = current_piece_bitboard[side] & piece_attacks_bb<Rook>(to, occupied);
    if (b)
    {
      const auto from = lsb(b);
      current_piece_bitboard[side] &= ~bit(from);
      return std::optional<Square>(from);
    }
    current_piece[side]++;
    current_piece_bitboard[side] = queens(side);
    [[fallthrough]];
  case Queen:
    b = current_piece_bitboard[side] & piece_attacks_bb<Queen>(to, occupied);
    if (b)
    {
      const auto from = lsb(b);
      current_piece_bitboard[side] &= ~bit(from);
      return std::optional<Square>(from);
    }
    current_piece[side]++;
    current_piece_bitboard[side] = king(side);
    [[fallthrough]];
  case King:
    b = current_piece_bitboard[side] & piece_attacks_bb<King>(to);
    if (b)
    {
      const auto from = lsb(b);
      current_piece_bitboard[side] &= ~bit(from);
      return std::optional<Square>(from);
    }
    break;
  }

  return std::nullopt;
}

void Board::init_see_move() {
  current_piece.fill(Pawn);
  current_piece_bitboard[WHITE] = piece[Pawn];
  current_piece_bitboard[BLACK] = piece[Pawn | 8];
}
