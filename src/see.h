#pragma once

#include <cstdint>

class See {
public:
  See(Game *game) : board_(game->board) {}

  int seeMove(const uint32_t move) {
    int score;
    board_.makeMove(move);

    if (!board_.isAttacked(board_.king_square[moveSide(move)], moveSide(move) ^ 1))
    {
      initSeeMove();
      score = seeRec(materialChange(move), nextToCapture(move), moveTo(move), moveSide(move) ^ 1);
    } else
    { score = SEE_INVALID_SCORE; }
    board_.unmakeMove(move);
    return score;
  }

  int seeLastMove(const uint32_t move) {
    initSeeMove();
    return seeRec(materialChange(move), nextToCapture(move), moveTo(move), moveSide(move) ^ 1);
  }

private:
  constexpr materialChange(const uint32_t move) {
    return (isCapture(move) ? piece_value(moveCaptured(move)) : 0) + (isPromotion(move) ? (piece_value(movePromoted(move)) - piece_value(Pawn)) : 0);
  }

  int nextToCapture(const uint32_t move) { return isPromotion(move) ? movePromoted(move) : movePiece(move); }

  int seeRec(const int material_change, const int next_to_capture, const uint64_t to, const int side_to_move) {
    uint64_t from;
    uint32_t move;

    do
    {
      if (!lookupBestAttacker(to, side_to_move, from))
      {
        return material_change;
      }

      if ((current_piece[side_to_move] == Pawn) && (rankOf(to) == 0 || rankOf(to) == 7))
      {
        initMove(move, current_piece[side_to_move] | (side_to_move << 3), next_to_capture, from, to, PROMOTION | CAPTURE, Queen | (side_to_move << 3));
      } else
      { initMove(move, current_piece[side_to_move] | (side_to_move << 3), next_to_capture, from, to, CAPTURE, 0); }
      board_.makeMove(move);

      if (!board_.isAttacked(board_.king_square[side_to_move], side_to_move ^ 1))
      {
        break;
      }
      board_.unmakeMove(move);
    } while (1);

    int score = -seeRec(materialChange(move), nextToCapture(move), moveTo(move), moveSide(move) ^ 1);

    board_.unmakeMove(move);

    return (score < 0) ? material_change + score : material_change;
  }

  __forceinline bool lookupBestAttacker(const uint64_t to, const int side, uint64_t &from) {// "Best" == "Lowest piece value"
    switch (current_piece[side])
    {
    case Pawn:
      if (current_piece_bitboard[side] & pawn_captures[to | ((side ^ 1) << 6)])
      {
        from = lsb(current_piece_bitboard[side] & pawn_captures[to | ((side ^ 1) << 6)]);
        current_piece_bitboard[side] &= ~bbSquare(from);
        return true;
      }
      current_piece[side]++;
      current_piece_bitboard[side] = board_.knights(side);

    [[fallthrough]]
    case Knight:
      if (current_piece_bitboard[side] & knightAttacks(to))
      {
        from = lsb(current_piece_bitboard[side] & knightAttacks(to));
        current_piece_bitboard[side] &= ~bbSquare(from);
        return true;
      }
      current_piece[side]++;
      current_piece_bitboard[side] = board_.bishops(side);

    [[fallthrough]]

    case Bishop:
      if (current_piece_bitboard[side] & bishopAttacks(to, board_.occupied))
      {
        from = lsb(current_piece_bitboard[side] & bishopAttacks(to, board_.occupied));
        current_piece_bitboard[side] &= ~bbSquare(from);
        return true;
      }
      current_piece[side]++;
      current_piece_bitboard[side] = board_.rooks(side);

    [[fallthrough]]
    case Rook:
      if (current_piece_bitboard[side] & rookAttacks(to, board_.occupied))
      {
        from = lsb(current_piece_bitboard[side] & rookAttacks(to, board_.occupied));
        current_piece_bitboard[side] &= ~bbSquare(from);
        return true;
      }
      current_piece[side]++;
      current_piece_bitboard[side] = board_.queens(side);

    [[fallthrough]]
    case Queen:
      if (current_piece_bitboard[side] & queenAttacks(to, board_.occupied))
      {
        from = lsb(current_piece_bitboard[side] & queenAttacks(to, board_.occupied));
        current_piece_bitboard[side] &= ~bbSquare(from);
        return true;
      }
      current_piece[side]++;
      current_piece_bitboard[side] = board_.king(side);

    [[fallthrough]]
    case King:
      if (current_piece_bitboard[side] & kingAttacks(to))
      {
        from = lsb(current_piece_bitboard[side] & kingAttacks(to));
        current_piece_bitboard[side] &= ~bbSquare(from);
        return true;
      }

    [[fallthrough]]
    default:
      return false;
    }
  }

protected:
  void initSeeMove() {
    current_piece[0]          = Pawn;
    current_piece_bitboard[0] = board_.piece[Pawn];
    current_piece[1]          = Pawn;
    current_piece_bitboard[1] = board_.piece[Pawn | 8];
  }

  uint64_t current_piece_bitboard[2];
  int current_piece[2];
  Board &board_;

  static constexpr int SEE_INVALID_SCORE = -5000;
};