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
#include "position.h"

enum Move : uint32_t;

struct Board {

  using PositionList = std::array<Position, MAXDEPTH * 3>;

  Board();
  explicit Board(std::string_view fen);

  static constexpr std::string_view kStartPosition = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

  void clear();

  bool make_move(Move m, bool check_legal, bool calculate_in_check);

  bool make_move(Move m, bool check_legal);

  void unmake_move();

  bool make_null_move();

  [[nodiscard]]
  uint64_t calculate_key() const;

  [[nodiscard]]
  bool is_repetition() const;

  [[nodiscard]]
  int64_t half_move_count() const;

  int new_game(std::string_view fen);

  int set_fen(std::string_view fen);

  [[nodiscard]]
  std::string get_fen() const;

  [[nodiscard]]
  bool setup_castling(std::string_view s);

  [[nodiscard]]
  std::string move_to_string(Move m) const;

  void print_moves() const;

  void add_piece(Piece pc, Square s);

  void perform_move(Move m);

  void unperform_move(Move m);

  [[nodiscard]]
  Piece get_piece(Square s) const;

  [[nodiscard]]
  PieceType get_piece_type(Square s) const;

  [[nodiscard]]
  Bitboard get_pinned_pieces(Color c, Square s);

  [[nodiscard]]
  bool is_attacked(Square s, Color c) const;

  void print() const;

  [[nodiscard]]
  Bitboard pieces(PieceType pt) const;

  [[nodiscard]]
  Bitboard pieces(PieceType pt, PieceType pt2) const;

  [[nodiscard]]
  Bitboard pieces(PieceType pt, Color c) const;

  [[nodiscard]]
  Bitboard pieces(PieceType pt, PieceType pt2, Color c) const;

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
  bool is_pawn_passed(Square s, Color c) const;

  [[nodiscard]]
  bool is_piece_on_file(PieceType pt, Square s, Color c) const;

  [[nodiscard]]
  bool is_pawn_isolated(Square s, Color c) const;

  [[nodiscard]]
  bool is_pawn_behind(Square s, Color c) const;

  [[nodiscard]]
  int see_move(Move m);

  [[nodiscard]]
  int see_last_move(Move m);

  [[nodiscard]]
  Key pawn_key() const;

  [[nodiscard]]
  Key key() const;

  [[nodiscard]]
  Material &material() const;

  [[nodiscard]]
  int &flags() const;

  [[nodiscard]]
  int castle_rights() const;

  [[nodiscard]]
  Square en_passant_square() const;

  [[nodiscard]]
  Color side_to_move() const;

  [[nodiscard]]
  Bitboard checkers() const;

  [[nodiscard]]
  bool in_check() const;

  [[nodiscard]]
  Bitboard pinned() const;

  void pinned(Bitboard v);

  [[nodiscard]]
  bool gives_check(Move m);

  [[nodiscard]]
  bool is_legal(Move m, Piece pc, Square from, MoveType mt);

  std::array<Bitboard, PIECE_NB> piece{};
  int plies{};
  int max_ply{};
  int search_depth{};
  std::array<int, SQ_NB> castle_rights_mask{};
  Position *pos;
  bool chess960;
  bool xfen;

private:

  void remove_piece(Square s);

  [[nodiscard]]
  bool is_occupied(Square s) const;

  [[nodiscard]]
  bool is_attacked_by_slider(Square s, Color c) const;

  [[nodiscard]]
  bool is_attacked_by_knight(Square s, Color c) const;

  [[nodiscard]]
  bool is_attacked_by_pawn(Square s, Color c) const;

  [[nodiscard]]
  bool is_attacked_by_king(Square s, Color c) const;

  [[nodiscard]]
  bool is_piece_on_square(PieceType pt, Square s, Color c);

  [[nodiscard]]
  int see_rec(int mat_change, Piece next_capture, Square to, Color c);

  [[nodiscard]]
  std::optional<Square> lookup_best_attacker(Square to, Color c);

  void init_see_move();

  void update_position(Position *p) const;

  [[nodiscard]]
  Bitboard attackers_to(Square s, Bitboard occ) const;

  [[nodiscard]]
  Bitboard attackers_to(Square s) const;

  std::array<Piece, SQ_NB> board{};
  std::array<Bitboard, COL_NB> occupied_by_side{};
  std::array<Bitboard, 2> current_piece_bitboard{};
  std::array<PieceType, 2> current_piece{};
  Bitboard occupied{};
  std::array<Square, COL_NB> king_square{};
  PositionList position_list{};
};

inline void Board::add_piece(const Piece pc, const Square s) {
  piece[pc] |= s;
  occupied_by_side[color_of(pc)] |= s;
  occupied |= s;
  board[s] = pc;

  if (type_of(pc) == KING)
    king_square[color_of(pc)] = s;
}

inline void Board::remove_piece(const Square s) {
  const auto pc   = board[s];
  const auto bbsq = ~bit(s);
  piece[pc] &= bbsq;
  occupied_by_side[color_of(pc)] &= bbsq;
  occupied &= bbsq;
  board[s] = NO_PIECE;
}

inline Piece Board::get_piece(const Square s) const {
  return board[s];
}

inline PieceType Board::get_piece_type(const Square s) const {
  return type_of(board[s]);
}

inline bool Board::is_occupied(const Square s) const {
  return occupied & s;
}

inline bool Board::is_attacked(const Square s, const Color c) const {
  return is_attacked_by_slider(s, c) || is_attacked_by_knight(s, c) || is_attacked_by_pawn(s, c) || is_attacked_by_king(s, c);
}

inline bool Board::is_attacked_by_knight(const Square s, const Color c) const {
  return (piece[make_piece(KNIGHT, c)] & piece_attacks_bb(KNIGHT, s)) != 0;
}

inline bool Board::is_attacked_by_pawn(const Square s, const Color c) const {
  return (piece[make_piece(PAWN, c)] & pawn_attacks_bb(~c, s)) != 0;
}

inline bool Board::is_attacked_by_king(const Square s, const Color c) const {
  return (piece[make_piece(KING, c)] & piece_attacks_bb(KING, s)) != 0;
}

inline Bitboard Board::pieces() const {
  return occupied;
}

inline Bitboard Board::pieces(const PieceType pt) const {
  return piece[make_piece(pt, WHITE)] | piece[make_piece(pt, BLACK)];
}

inline Bitboard Board::pieces(const PieceType pt, const PieceType pt2) const {
  return pieces(pt) | pieces(pt2);
}

inline Bitboard Board::pieces(const PieceType pt, const Color c) const {
  return piece[make_piece(pt, c)];
}

inline Bitboard Board::pieces(const PieceType pt, const PieceType pt2, const Color c) const {
  return piece[make_piece(pt, c)] | piece[make_piece(pt2, c)];
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

inline bool Board::is_pawn_passed(const Square s, const Color c) const {
  return (passed_pawn_front_span[c][s] & pieces(PAWN, ~c)) == 0;
}

inline bool Board::is_piece_on_square(const PieceType pt, const Square s, const Color c) {
  return (piece[make_piece(pt, c)] & s) != 0;
}

inline bool Board::is_piece_on_file(const PieceType pt, const Square s, const Color c) const {
  return (bb_file(s) & piece[make_piece(pt, c)]) != 0;
}

inline Key Board::pawn_key() const {
  return pos->pawn_structure_key;
}

inline Key Board::key() const {
  return pos->key;
}

inline Material &Board::material() const {
  return pos->material;
}

inline int &Board::flags() const {
  return pos->flags;
}

inline int Board::castle_rights() const {
  return pos->castle_rights;
}

inline Square Board::en_passant_square() const {
  return pos->en_passant_square;
}

inline Color Board::side_to_move() const {
  return pos->side_to_move;
}

inline Bitboard Board::checkers() const {
  return pos->checkers;
}

inline bool Board::in_check() const {
  return pos->in_check;
}

inline Bitboard Board::pinned() const {
  return pos->pinned_;
}

inline void Board::pinned(const Bitboard v) {
  pos->pinned_ = v;
}


