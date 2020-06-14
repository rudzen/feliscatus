#pragma once

#include "moves.h"
#include "material.h"

class HashEntry;

struct Position : public Moves {
  Position();

  void clear();

  [[nodiscard]]
  const uint32_t *string_to_move(const char *m);

  [[nodiscard]]
  bool is_castle_move(const char *m, int &castle_type) const;

  [[nodiscard]]
  bool is_draw() const;

  int reversible_half_move_count;
  Key pawn_structure_key;
  Key key;
  Material material;
  int null_moves_in_row;
  int pv_length;
  uint32_t last_move;
  int eval_score;
  int transp_score;
  int transp_depth;
  NodeType transp_type;
  uint32_t transp_move;
  int flags;
  HashEntry *transposition;
};
