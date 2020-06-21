#pragma once

#include <string_view>
#include "moves.h"
#include "material.h"

class HashEntry;

struct Position : public Moves {
  void clear();

  [[nodiscard]]
  const Move *string_to_move(std::string_view m);

  [[nodiscard]]
  bool is_draw() const;

  int reversible_half_move_count{};
  Key pawn_structure_key{};
  Key key{};
  Material material{};
  int null_moves_in_row{};
  int pv_length{};
  Move last_move{};
  int eval_score{};
  int transp_score{};
  int transp_depth{};
  NodeType transp_type{};
  Move transp_move{};
  int flags{};
  HashEntry *transposition{};
};
