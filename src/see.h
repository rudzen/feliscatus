#pragma once

#include <cstdint>
#include <array>
#include "types.h"
#include "square.h"

class Board;

struct See {
  See() = delete;
  explicit See(Board *board);

  int see_move(uint32_t move);

  int see_last_move(uint32_t move);

private:

  int see_rec(int mat_change, int next_capture, Square to, Color side_to_move);

  bool lookup_best_attacker(Square to, Color side, Square &from);

  void init_see_move();

  std::array<Bitboard, 2> current_piece_bitboard{};
  std::array<int, 2> current_piece{};
  Board *board_;
};