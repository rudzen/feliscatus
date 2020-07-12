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
#include <functional>

#include "types.h"

struct MoveData {
  Move move{};
  int score{};
  constexpr operator Move() const { return move; }
  void operator=(const Move m) { move = m; }
  constexpr auto operator<=> (const MoveData &rhs) {
    return score <=> rhs.score;
  }
};

struct Board;

enum MoveStage {
  TT_STAGE,
  CAPTURE_STAGE,
  QUIET_STAGE,
  END_STAGE
};

struct Moves final {

  explicit Moves(Board *board);

  void generate_moves(Move tt_move = MOVE_NONE, int flags = 0);

  void generate_captures_and_promotions();

  template<Color Us>
  void generate_moves(PieceType pt, Bitboard to_squares);

  void generate_pawn_moves(bool capture, Bitboard to_squares, Color c);

  [[nodiscard]]
  const MoveData *next_move();

  [[nodiscard]]
  int move_count() const;

private:
  void reset(Move m, int flags);

  void generate_hash_move();

  template<Color Us>
  void generate_captures_and_promotions();

  template<Color Us>
  void generate_quiet_moves();

  template<Color Us>
  void add_move(Piece pc, Square from, Square to, MoveType mt, Piece promoted = NO_PIECE);

  template<Color Us, PieceType Pt>
  void add_piece_moves(Bitboard to_squares);

  template<Color Us>
  void add_moves(Bitboard to_squares);

  template<Color Us>
  void add_moves(PieceType pt, Square from, Bitboard attacks);

  template<Color Us>
  void add_pawn_quiet_moves(Bitboard to_squares);

  template<Color Us>
  void add_pawn_capture_moves(Bitboard to_squares);

  template<Color Us>
  void add_pawn_moves(Bitboard to_squares, Direction d, MoveType mt);

  template<Color Us>
  void add_castle_move(Square from, Square to);

  template<Color Us>
  [[nodiscard]]
  const MoveData *get_next_move();

  template<Color Us>
  [[nodiscard]]
  bool can_castle_short() const;

  template<Color Us>
  [[nodiscard]]
  bool can_castle_long() const;

  std::array<MoveData, 256> move_list{};
  int iteration_{};
  int stage_{};
  int max_stage_{};
  int number_moves_{};
  Move transp_move_{};
  int move_flags_{};
  Board *b{};
};

inline int Moves::move_count() const {
  return number_moves_;
}
