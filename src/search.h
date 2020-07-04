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

#include <atomic>
#include <optional>

#include "datapool.h"
#include "board.h"

struct Position;

struct Search final : MoveSorter {
  Search() = delete;
  Search(Board *board) : b(board), data_(board->data()), verbosity(data_->index() == 0) {
    if (!verbosity)
      stop_search.store(false);
  }

  int go(SearchLimits &limits);

  void stop();

  void run();

private:
  template<NodeType NT, bool PV>
  int search(int depth, int alpha, int beta);

  template<NodeType NT, bool PV>
  int search_next_depth(int depth, int alpha, int beta);

  template<bool PV>
  Move get_singular_move(int depth);

  bool search_fail_low(int depth, int alpha, Move exclude);

  [[nodiscard]]
  bool should_try_null_move(int beta) const;

  template<NodeType NT, bool PV>
  std::optional<int> next_depth_not_pv(int depth, int move_count, const MoveData *move_data, int alpha, int &best_score) const;

  int next_depth_pv(Move singular_move, int depth, const MoveData *md) const;

  template<bool PV>
  int search_quiesce(int alpha, int beta, int qs_ply);

  bool make_move_and_evaluate(Move m, int alpha, int beta);

  void unmake_move();

  void check_sometimes(uint64_t nodes);

  void check_time();

  [[nodiscard]]
  int is_analysing() const;

  template<NodeType NT>
  void update_pv(Move m, int score, int depth) const;

  void init_search(SearchLimits &limits);

  void sort_move(MoveData &md) override;

  [[nodiscard]]
  int store_search_node_score(int score, int depth, NodeType nt, Move m) const;

  [[nodiscard]]
  int draw_score() const;

  void store_hash(int depth, int score, NodeType nt, Move m) const;

  [[nodiscard]]
  bool move_is_easy() const;

  std::atomic_bool stop_search;
  std::array<int, COL_NB> draw_score_{};
  Board *b;
  Position *pos{};
  Data *data_;
  bool verbosity{};
};
