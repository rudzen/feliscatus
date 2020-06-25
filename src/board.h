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

#pragma once

#include <cstdint>
#include <array>
#include <optional>

#include "types.h"
#include "magic.h"

enum Move : uint32_t;

struct Board {
  void clear();

  void add_piece(int p, Square sq);

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
  Bitboard pieces() const;

  [[nodiscard]]
  Bitboard pieces(Color c) const;

  [[nodiscard]]
  Bitboard king(Color c) const;

  [[nodiscard]]
  Square king_sq(Color c) const;

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

  std::array<Bitboard, 2 << 3> piece{};
  std::array<int, sq_nb> board{};

private:

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

  std::array<Bitboard, COL_NB> occupied_by_side{};
  std::array<Bitboard, 2> current_piece_bitboard{};
  std::array<int, 2> current_piece{};
  Bitboard occupied{};
  std::array<Square, COL_NB> king_square{};
};

inline void Board::add_piece(const int p, const Square sq) {
  piece[p] |= sq;
  occupied_by_side[p >> 3] |= sq;
  occupied |= sq;
  board[sq] = p;

  if ((p & 7) == King)
    king_square[p >> 3] = sq;
}

inline void Board::remove_piece(const int p, const Square sq) {
  const auto bbsq = bit(sq);
  piece[p] &= ~bbsq;
  occupied_by_side[p >> 3] &= ~bbsq;
  occupied &= ~bbsq;
  board[sq] = NoPiece;
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
  return (piece[Knight | (side << 3)] & piece_attacks_bb(Knight, sq)) != 0;
}

inline bool Board::is_attacked_by_pawn(const Square sq, const Color side) const {
  return (piece[Pawn | side << 3] & pawn_attacks_bb(~side, sq)) != 0;
}

inline bool Board::is_attacked_by_king(const Square sq, const Color side) const {
  return (piece[King | side << 3] & piece_attacks_bb(King, sq)) != 0;
}

inline Bitboard Board::pieces() const {
  return occupied;
}

inline Bitboard Board::pieces(const int p, const Color side) const {
  return piece[p | side << 3];
}

inline Bitboard Board::pieces(const Color c) const {
  return occupied_by_side[c];
}

inline Bitboard Board::king(const Color c) const {
  return bit(king_sq(c));
}

inline Square Board::king_sq(const Color c) const {
  return king_square[c];
}

inline bool Board::is_pawn_passed(const Square sq, const Color side) const {
  return (passed_pawn_front_span[side][sq] & pieces(Pawn, ~side)) == 0;
}

inline bool Board::is_piece_on_square(const int p, const Square sq, const Color side) {
  return (piece[p | (side << 3)] & sq) != 0;
}

inline bool Board::is_piece_on_file(const int p, const Square sq, const Color side) const {
  return (bb_file(sq) & piece[p | (side << 3)]) != 0;
}
