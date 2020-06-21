
#pragma once

#include <cstdint>
#include <array>
#include <optional>
#include "types.h"
#include "square.h"
#include "bitboard.h"

enum Move : uint32_t;

struct Board {
  void clear();

  void add_piece(int p, Color side, Square sq);

  void make_move(Move m);

  void unmake_move(Move m);

  [[nodiscard]]
  int get_piece(Square sq) const;

  [[nodiscard]]
  int get_piece_type(Square sq) const;

  [[nodiscard]]
  Bitboard get_pinned_pieces(Color side, Square sq);

  [[nodiscard]]
  bool is_attacked(Square sq, Color side) const;

  void print() const;

  [[nodiscard]]
  Bitboard pieces(int p, Color side) const;

  [[nodiscard]]
  Bitboard pawns(Color side) const;

  [[nodiscard]]
  Bitboard knights(Color side) const;

  [[nodiscard]]
  Bitboard bishops(Color side) const;

  [[nodiscard]]
  Bitboard rooks(Color side) const;

  [[nodiscard]]
  Bitboard queens(Color side) const;

  [[nodiscard]]
  Bitboard king(Color side) const;

  std::array<Bitboard, 2 << 3> piece{};
  std::array<Bitboard, COL_NB> occupied_by_side{};
  std::array<int, 64> board{};
  std::array<Square, COL_NB> king_square{};
  Bitboard queen_attacks{};
  Bitboard occupied{};

  [[nodiscard]]
  bool is_passed_pawn_move(Move m) const;

  [[nodiscard]]
  bool is_pawn_passed(Square sq, Color side) const;

  [[nodiscard]]
  bool is_piece_on_file(int p, Square sq, Color side) const;

  [[nodiscard]]
  bool is_pawn_isolated(Square sq, Color side) const;

  [[nodiscard]]
  bool is_pawn_behind(Square sq, Color side) const;

  [[nodiscard]]
  int see_move(Move move);

  [[nodiscard]]
  int see_last_move(Move move);

private:

  void add_piece(int p, Square sq);

  void remove_piece(int p, Square sq);

  [[nodiscard]]
  bool is_occupied(Square sq) const;

  [[nodiscard]]
  bool is_attacked_by_slider(Square sq, Color side) const;

  [[nodiscard]]
  bool is_attacked_by_knight(Square sq, Color side) const;

  [[nodiscard]]
  bool is_attacked_by_pawn(Square sq, Color side) const;

  [[nodiscard]]
  bool is_attacked_by_king(Square sq, Color side) const;

  [[nodiscard]]
  bool is_piece_on_square(int p, Square sq, Color side);

  [[nodiscard]]
  int see_rec(int mat_change, int next_capture, Square to, Color side_to_move);

  [[nodiscard]]
  std::optional<Square> lookup_best_attacker(Square to, Color side);

  void init_see_move();

  std::array<Bitboard, 2> current_piece_bitboard{};
  std::array<int, 2> current_piece{};
};

inline void Board::add_piece(const int p, const Color side, const Square sq) {
  const auto pc = p | (side << 3);
  piece[pc] |= sq;
  occupied_by_side[side] |= sq;
  occupied |= sq;
  board[sq] = pc;

  if (p == King)
      king_square[side] = sq;
}

inline void Board::remove_piece(const int p, const Square sq) {
  const auto bbsq = bit(sq);
  piece[p] &= ~bbsq;
  occupied_by_side[p >> 3] &= ~bbsq;
  occupied &= ~bbsq;
  board[sq] = NoPiece;
}

inline void Board::add_piece(const int p, const Square sq) {
  piece[p] |= sq;
  occupied_by_side[p >> 3] |= sq;
  occupied |= sq;
  board[sq] = p;
}

inline int Board::get_piece(const Square sq) const {
  return board[sq];
}

inline int Board::get_piece_type(const Square sq) const {
  return board[sq] & 7;
}

inline bool Board::is_occupied(const Square sq) const {
  return occupied & sq;
}

inline bool Board::is_attacked(const Square sq, const Color side) const {
  return is_attacked_by_slider(sq, side) || is_attacked_by_knight(sq, side) || is_attacked_by_pawn(sq, side) || is_attacked_by_king(sq, side);
}

inline bool Board::is_attacked_by_knight(const Square sq, const Color side) const {
  return (piece[Knight | (side << 3)] & knight_attacks[sq]) != 0;
}

inline bool Board::is_attacked_by_pawn(const Square sq, const Color side) const {
  return (piece[Pawn | side << 3] & pawn_captures[sq | (~side) << 6]) != 0;
}

inline bool Board::is_attacked_by_king(const Square sq, const Color side) const {
  return (piece[King | side << 3] & king_attacks[sq]) != 0;
}

inline Bitboard Board::pieces(const int p, const Color side) const {
  return piece[p | side << 3];
}

inline Bitboard Board::pawns(const Color side) const {
  return piece[Pawn | side << 3];
}

inline Bitboard Board::knights(const Color side) const {
  return piece[Knight | side << 3];
}

inline Bitboard Board::bishops(const Color side) const {
  return piece[Bishop | side << 3];
}

inline Bitboard Board::rooks(const Color side) const {
  return piece[Rook | side << 3];
}

inline Bitboard Board::queens(const Color side) const {
  return piece[Queen | side << 3];
}

inline Bitboard Board::king(const Color side) const {
  return piece[King | side << 3];
}

inline bool Board::is_pawn_passed(const Square sq, const Color side) const {
  return (passed_pawn_front_span[side][sq] & pawns(~side)) == 0;
}

inline bool Board::is_piece_on_square(const int p, const Square sq, const Color side) {
  return (piece[p | (side << 3)] & sq) != 0;
}

inline bool Board::is_piece_on_file(const int p, const Square sq, const Color side) const {
  return (bb_file(sq) & piece[p | (side << 3)]) != 0;
}
