// Copyright (C) 2008-2016 Gunnar Harms (Bobcat author)
// Copyright (C) 2017      FireFather (Tomcat author)
// Copyright (C) 2020-2025 Rudy Alex Kohn
// See end of file for extended copyright information.

#pragma once

#include <vector>
#include <filesystem>
#include <memory>
#include <tuner/pgn_player.hpp>
#include <cli/cli_parser.hpp>
#include <felis/bitboard.hpp>
#include <felis/moves.hpp>

struct Board;
struct FileResolver;

namespace eval
{

struct Node;
struct Param;
struct ParamIndexRecord;

class PGNPlayer : public pgn::PGNPlayer
{
public:
  PGNPlayer();

  virtual ~PGNPlayer() = default;

  void read_pgn_database() override;

  void read_san_move() override;

  void read_game_termination() override;

  void read_comment1() override;

  void printProgress(bool force) const;

  std::vector<Node> all_selected_nodes_;

private:
  std::vector<Node> current_game_nodes_;
  std::int64_t all_nodes_count_{};
};

class Tune final {
public:
  explicit Tune(std::unique_ptr<Board> board, const ParserSettings *settings);

  double
    e(const std::vector<Node> &nodes, const std::vector<Param> &params,
      const std::vector<ParamIndexRecord> &paramsIndex, double k);

  void makeQuiet(std::vector<Node> &nodes);

  [[nodiscard]] int score(Color c) const;

  [[nodiscard]] int quiesceScore(int alpha, int beta, bool storePv, int ply) const;

  [[nodiscard]] bool makeMove(Move m, int ply) const;

  void unmakeMove() const;

  void playPv() const;

  void updatePv(Move m, int score, int ply) const;

private:
  std::unique_ptr<Board> b;
  bool score_static_;
};

}   // namespace eval

// Feliscatus, a UCI chess playing engine derived from Tomcat 1.0 (Bobcat 8.0)
// Copyright (C) 2008-2016 Gunnar Harms (Bobcat author)
// Copyright (C) 2017      FireFather (Tomcat author)
// Copyright (C) 2020-2022 Rudy Alex Kohn
//
// Feliscatus is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Feliscatus is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Feliscatus.  If not, see <http://www.gnu.org/licenses/>.