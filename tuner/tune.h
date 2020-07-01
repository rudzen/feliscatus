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

#include <utility>
#include <vector>
#include <string>
#include <filesystem>
#include <memory>
#include <map>

#include "../src/bitboard.h"
#include "pgn_player.h"
#include "../src/search.h"
#include "../cli/cli_parser.h"

struct Board;
struct PVEntry;
struct FileResolver;

namespace eval {

struct Node;
struct Param;
struct ParamIndexRecord;

class PGNPlayer : public pgn::PGNPlayer {
public:
  PGNPlayer();

  virtual ~PGNPlayer() = default;

  void read_pgn_database() override;

  void read_san_move() override;

  void read_game_termination() override;

  void read_comment1() override;

  void print_progress(bool force) const;

  std::vector<Node> all_selected_nodes_;

private:
  std::vector<Node> current_game_nodes_;
  int64_t all_nodes_count_{};
};

class Tune final : public MoveSorter {
public:
  explicit Tune(std::unique_ptr<Board> game, const ParserSettings *settings);

  double e(const std::vector<Node> &nodes, const std::vector<Param> &params, const std::vector<ParamIndexRecord> &params_index, double K);

  void make_quiet(std::vector<Node> &nodes);

  int get_score(Color side);

  int get_quiesce_score(int alpha, int beta, bool store_pv, int ply);

  bool make_move(Move m, int ply);

  void unmake_move() const;

  void play_pv();

  void update_pv(Move move, int score, int ply);

  void sort_move(MoveData &move_data) override;

private:
  std::unique_ptr<Board> board_;
  PawnHashTable pawn_table_{};

  PVEntry pv[MAXDEPTH][MAXDEPTH]{};
  std::array<int, MAXDEPTH> pv_length{};
  bool score_static_;
};

}// namespace eval