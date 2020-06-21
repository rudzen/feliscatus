#pragma once

#include <array>
#include <cstdint>
#include "types.h"

struct MoveData {
  Move move;
  int score;
};

struct MoveSorter {
  virtual ~MoveSorter() = default;
  virtual void sort_move(MoveData &move_data) = 0;
};

struct Board;

class Moves {
public:
  void generate_moves(MoveSorter *sorter = nullptr, Move tt_move = MOVE_NONE, int flags = 0);

  void generate_captures_and_promotions(MoveSorter *sorter);

  void generate_moves(int piece, Bitboard to_squares);

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
  void reset(MoveSorter *sorter, Move move, int flags);

  void generate_hash_move();

  void generate_captures_and_promotions();

  void generate_quiet_moves();

  void add_move(int piece, Square from, Square to, MoveType type, int promoted = 0);

  void add_moves(Bitboard to_squares);

  void add_moves(int piece, Square from, Bitboard attacks);

  void add_pawn_quiet_moves(Bitboard to_squares);

  void add_pawn_capture_moves(Bitboard to_squares);

  void add_pawn_moves(Bitboard to_squares, Direction distance, MoveType type);

  void add_castle_move(Square from, Square to);

  [[nodiscard]]
  bool gives_check(Move m) const;

  [[nodiscard]]
  bool is_legal(Move m, int piece, Square from, MoveType type) const;

  [[nodiscard]]
  bool can_castle_short() const;

  [[nodiscard]]
  bool can_castle_long() const;

  int iteration{};
  int stage{};
  int max_stage{};
  int number_moves{};
  Bitboard pinned{};
  MoveSorter *move_sorter{};
  Move transp_move{};
  int move_flags{};
};

inline int Moves::move_count() const {
  return number_moves;
}

inline void Moves::goto_move(const int pos) {
  iteration = pos;
}
