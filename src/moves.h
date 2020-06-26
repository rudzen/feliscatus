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

#include <array>
#include <cstdint>
#include <optional>

#include "types.h"

struct MoveData {
  Move move;
  int score;
  operator Move() const { return move; }
  void operator=(const Move m) { move = m; }  // NOLINT(misc-unconventional-assign-operator)
};

inline bool operator<(const MoveData &f, const MoveData &s) {
  return f.score < s.score;
}

struct MoveSorter {
  virtual ~MoveSorter() = default;
  virtual void sort_move(MoveData &move_data) = 0;
};

struct Board;

struct Moves {
  void generate_moves(std::optional<MoveSorter *> sorter = std::nullopt, Move tt_move = MOVE_NONE, int flags = 0);

  void generate_captures_and_promotions(std::optional<MoveSorter *> sorter);

  void generate_moves(PieceType pt, Bitboard to_squares);

  void generate_pawn_moves(bool capture, Bitboard to_squares);

  [[nodiscard]]
  MoveData *next_move();

  [[nodiscard]]
  int move_count() const;

  void goto_move(int pos);

  [[nodiscard]]
  bool is_pseudo_legal(Move m) const;

  std::array<MoveData, 256> move_list{};

  Board *b{};
  Color side_to_move{};
  int castle_rights{};
  bool in_check{};
  Square en_passant_square{};

private:
  void reset(std::optional<MoveSorter *> sorter, Move move, int flags);

  void generate_hash_move();

  void generate_captures_and_promotions();

  void generate_quiet_moves();

  void add_move(Piece piece, Square from, Square to, MoveType type, Piece promoted = NoPiece);

  void add_moves(Bitboard to_squares);

  void add_moves(PieceType pt, Square from, Bitboard attacks);

  void add_pawn_quiet_moves(Bitboard to_squares);

  void add_pawn_capture_moves(Bitboard to_squares);

  void add_pawn_moves(Bitboard to_squares, Direction distance, MoveType type);

  void add_castle_move(Square from, Square to);

  [[nodiscard]]
  bool gives_check(Move m) const;

  [[nodiscard]]
  bool is_legal(Move m, Piece piece, Square from, MoveType type) const;

  [[nodiscard]]
  bool can_castle_short() const;

  [[nodiscard]]
  bool can_castle_long() const;

  int iteration_{};
  int stage_{};
  int max_stage_{};
  int number_moves_{};
  Bitboard pinned_{};
  std::optional<MoveSorter *> move_sorter_;
  Move transp_move_{};
  int move_flags_{};
};

inline int Moves::move_count() const {
  return number_moves_;
}

inline void Moves::goto_move(const int pos) {
  iteration_ = pos;
}
