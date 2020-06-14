#pragma once

#include <cstdint>
#include <array>

class See {
public:
  explicit See(Game *game) : board_(game->board) {}

  int see_move(const uint32_t move) {
    int score;
    board_.make_move(move);

    if (!board_.is_attacked(board_.king_square[move_side(move)], ~move_side(move)))
    {
      init_see_move();
      score = see_rec(material_change(move), next_to_capture(move), move_to(move), ~move_side(move));
    } else
      score = SEE_INVALID_SCORE;

    board_.unmake_move(move);
    return score;
  }

  int see_last_move(const uint32_t move) {
    init_see_move();
    return see_rec(material_change(move), next_to_capture(move), move_to(move), ~move_side(move));
  }

private:
  static constexpr int material_change(const uint32_t move) {
    return (is_capture(move) ? piece_value(move_captured(move)) : 0) + (is_promotion(move) ? (piece_value(move_promoted(move)) - piece_value(Pawn)) : 0);
  }

  static constexpr int next_to_capture(const uint32_t move) { return is_promotion(move) ? move_promoted(move) : move_piece(move); }

  int see_rec(const int mat_change, const int next_capture, const Square to, const Color side_to_move) {
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

      board_.make_move(move);

      if (!board_.is_attacked(board_.king_square[side_to_move], ~side_to_move))
        break;

      board_.unmake_move(move);
    } while (true);

    const auto score = -see_rec(material_change(move), next_to_capture(move), move_to(move), ~move_side(move));

    board_.unmake_move(move);

    return (score < 0) ? mat_change + score : mat_change;
  }

  bool lookup_best_attacker(const Square to, const Color side, Square &from) {// "Best" == "Lowest piece value"
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
      current_piece_bitboard[side] = board_.knights(side);

    [[fallthrough]];
    case Knight:
      if (current_piece_bitboard[side] & knightAttacks(to))
      {
        from = lsb(current_piece_bitboard[side] & knightAttacks(to));
        current_piece_bitboard[side] &= ~bit(from);
        return true;
      }
      current_piece[side]++;
      current_piece_bitboard[side] = board_.bishops(side);

    [[fallthrough]];

    case Bishop:
      if (current_piece_bitboard[side] & bishopAttacks(to, board_.occupied))
      {
        from = lsb(current_piece_bitboard[side] & bishopAttacks(to, board_.occupied));
        current_piece_bitboard[side] &= ~bit(from);
        return true;
      }
      current_piece[side]++;
      current_piece_bitboard[side] = board_.rooks(side);

    [[fallthrough]];
    case Rook:
      if (current_piece_bitboard[side] & rookAttacks(to, board_.occupied))
      {
        from = lsb(current_piece_bitboard[side] & rookAttacks(to, board_.occupied));
        current_piece_bitboard[side] &= ~bit(from);
        return true;
      }
      current_piece[side]++;
      current_piece_bitboard[side] = board_.queens(side);

    [[fallthrough]];
    case Queen:
      if (current_piece_bitboard[side] & queenAttacks(to, board_.occupied))
      {
        from = lsb(current_piece_bitboard[side] & queenAttacks(to, board_.occupied));
        current_piece_bitboard[side] &= ~bit(from);
        return true;
      }
      current_piece[side]++;
      current_piece_bitboard[side] = board_.king(side);

    [[fallthrough]];
    case King:
      if (current_piece_bitboard[side] & kingAttacks(to))
      {
        from = lsb(current_piece_bitboard[side] & kingAttacks(to));
        current_piece_bitboard[side] &= ~bit(from);
        return true;
      }

    [[fallthrough]];
    default:
      return false;
    }
  }

protected:
  void init_see_move() {
    current_piece.fill(Pawn);
    current_piece_bitboard[WHITE] = board_.piece[Pawn];
    current_piece_bitboard[BLACK] = board_.piece[Pawn | 8];
  }

  std::array<Bitboard, 2> current_piece_bitboard{};
  std::array<int, 2> current_piece{};
  Board &board_;

  static constexpr int SEE_INVALID_SCORE = -5000;
};