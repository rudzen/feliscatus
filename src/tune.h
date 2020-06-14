#pragma once

#include <utility>
#include <vector>
#include "moves.h"
#include "pgn_player.h"
#include "search.h"

class Game;
struct See;
class Eval;
struct PVEntry;

namespace eval {

struct Node {
  Node(const char *fen) : fen_(fen) {}

  std::string fen_;
  double result_{};
};

inline bool x;

struct Param {
  Param(std::string name, int &value, const int initial_value, const int step) : name_(std::move(name)), initial_value_(initial_value), value_(value), step_(step) {
    if (x)
      value = initial_value;
  }

  std::string name_;
  int initial_value_;
  int &value_;
  int step_;
};

struct ParamIndexRecord {
  size_t idx_;
  double improved_;
};

inline bool operator<(const ParamIndexRecord &lhs, const ParamIndexRecord &rhs) {
  return lhs.improved_ >= rhs.improved_;
}

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
  int64_t all_nodes_count_;
};


// TODO : Create command line parameters to set which values should be tuned.

class Tune : public MoveSorter {
public:
  Tune(Game *game, See *see, Eval *eval);

  virtual ~Tune();

  static void init_eval(std::vector<Param> &params);

  double e(const std::vector<Node> &nodes, const std::vector<Param> &params, const std::vector<ParamIndexRecord> &params_index, double K);

  void make_quiet(std::vector<Node> &nodes);

  int get_score(int side);

  int get_quiesce_score(int alpha, int beta, bool store_pv, int ply);

  bool make_move(uint32_t m, int ply);

  void unmake_move() const;

  void play_pv();

  void update_pv(uint32_t move, int score, int ply);

  void sort_move(MoveData &move_data) override;

private:
  Game *game_;
  See *see_;
  Eval *eval_;

  PVEntry pv[128][128]{};
  int pv_length[128]{};
  bool score_static_;
};

}// namespace eval