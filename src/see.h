#pragma once

#include <cstdint>
#include <array>

class See {
public:
  explicit See(Game *game) : board_(game->board) {}

  int see_move(const uint32_t move) {
    int score;
    board_.make_move(move);

    if (!board_.is_attacked(board_.king_square[moveSide(move)], moveSide(move) ^ 1))
    {
      init_see_move();
      score = see_rec(material_change(move), next_to_capture(move), moveTo(move), moveSide(move) ^ 1);
    } else
      score = SEE_INVALID_SCORE;

    board_.unmake_move(move);
    return score;
  }

  int see_last_move(const uint32_t move) {
    init_see_move();
    return see_rec(material_change(move), next_to_capture(move), moveTo(move), moveSide(move) ^ 1);
  }

private:
  static constexpr int material_change(const uint32_t move) {
    return (isCapture(move) ? piece_value(moveCaptured(move)) : 0) + (isPromotion(move) ? (piece_value(movePromoted(move)) - piece_value(Pawn)) : 0);
  }

  static constexpr int next_to_capture(const uint32_t move) { return isPromotion(move) ? movePromoted(move) : movePiece(move); }

  int see_rec(const int mat_change, const int next_capture, const uint64_t to, const int side_to_move) {
    uint64_t from;
    uint32_t move;

    do
    {
      if (!lookup_best_attacker(to, side_to_move, from))
        return mat_change;

      if ((current_piece[side_to_move] == Pawn) && (rank_of(to) == 0 || rank_of(to) == 7))
        initMove(move, current_piece[side_to_move] | (side_to_move << 3), next_capture, from, to, PROMOTION | CAPTURE, Queen | (side_to_move << 3));
      else
        initMove(move, current_piece[side_to_move] | (side_to_move << 3), next_capture, from, to, CAPTURE, 0);

      board_.make_move(move);

      if (!board_.is_attacked(board_.king_square[side_to_move], side_to_move ^ 1))
        break;

      board_.unmake_move(move);
    } while (true);

    const auto score = -see_rec(material_change(move), next_to_capture(move), moveTo(move), moveSide(move) ^ 1);

    board_.unmake_move(move);

    return (score < 0) ? mat_change + score : mat_change;
  }

  bool lookup_best_attacker(const uint64_t to, const int side, uint64_t &from) {// "Best" == "Lowest piece value"
    switch (current_piece[side])
    {
    case Pawn:
      if (current_piece_bitboard[side] & pawn_captures[to | ((side ^ 1) << 6)])
      {
        from = lsb(current_piece_bitboard[side] & pawn_captures[to | ((side ^ 1) << 6)]);
        current_piece_bitboard[side] &= ~bb_square(from);
        return true;
      }
      current_piece[side]++;
      current_piece_bitboard[side] = board_.knights(side);

    [[fallthrough]];
    case Knight:
      if (current_piece_bitboard[side] & knightAttacks(to))
      {
        from = lsb(current_piece_bitboard[side] & knightAttacks(to));
        current_piece_bitboard[side] &= ~bb_square(from);
        return true;
      }
      current_piece[side]++;
      current_piece_bitboard[side] = board_.bishops(side);

    [[fallthrough]];

    case Bishop:
      if (current_piece_bitboard[side] & bishopAttacks(to, board_.occupied))
      {
        from = lsb(current_piece_bitboard[side] & bishopAttacks(to, board_.occupied));
        current_piece_bitboard[side] &= ~bb_square(from);
        return true;
      }
      current_piece[side]++;
      current_piece_bitboard[side] = board_.rooks(side);

    [[fallthrough]];
    case Rook:
      if (current_piece_bitboard[side] & rookAttacks(to, board_.occupied))
      {
        from = lsb(current_piece_bitboard[side] & rookAttacks(to, board_.occupied));
        current_piece_bitboard[side] &= ~bb_square(from);
        return true;
      }
      current_piece[side]++;
      current_piece_bitboard[side] = board_.queens(side);

    [[fallthrough]];
    case Queen:
      if (current_piece_bitboard[side] & queenAttacks(to, board_.occupied))
      {
        from = lsb(current_piece_bitboard[side] & queenAttacks(to, board_.occupied));
        current_piece_bitboard[side] &= ~bb_square(from);
        return true;
      }
      current_piece[side]++;
      current_piece_bitboard[side] = board_.king(side);

    [[fallthrough]];
    case King:
      if (current_piece_bitboard[side] & kingAttacks(to))
      {
        from = lsb(current_piece_bitboard[side] & kingAttacks(to));
        current_piece_bitboard[side] &= ~bb_square(from);
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
    current_piece_bitboard[0] = board_.piece[Pawn];
    current_piece_bitboard[1] = board_.piece[Pawn | 8];
  }

  std::array<uint64_t, 2> current_piece_bitboard{};
  std::array<int, 2> current_piece{};
  Board &board_;

  static constexpr int SEE_INVALID_SCORE = -5000;
};