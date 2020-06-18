#pragma once

#include <utility>
#include <vector>
#include <string>
#include <fmt/format.h>
#include <docopt/docopt.h>
#include <map>
#include "../src/bitboard.h"
#include "pgn_player.h"
#include "../src/search.h"

class Game;
struct PVEntry;


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


// TODO : Create command line parameters to set which values should be tuned.

class Tune final : public MoveSorter {
public:
  explicit Tune(Game *game, std::string_view input, std::string_view output, const std::map<std::string, docopt::value> &args);

  double e(const std::vector<Node> &nodes, const std::vector<Param> &params, const std::vector<ParamIndexRecord> &params_index, double K);

  void make_quiet(std::vector<Node> &nodes);

  int get_score(Color side);

  int get_quiesce_score(int alpha, int beta, bool store_pv, int ply);

  bool make_move(uint32_t m, int ply);

  void unmake_move() const;

  void play_pv();

  void update_pv(uint32_t move, int score, int ply);

  void sort_move(MoveData &move_data) override;

private:
  Game *game_;
  PawnHashTable pawn_table_{};

  PVEntry pv[128][128]{};
  int pv_length[128]{};
  bool score_static_;
};

}// namespace eval