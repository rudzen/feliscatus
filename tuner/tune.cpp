#include <fmt/format.h>
#include <fstream>
#include <unordered_map>
#include <docopt/docopt.h>
#include <string>
#include "tune.h"
#include "../src/game.h"
#include "../src/eval.h"
#include "../src/parameters.h"
#include "../src/util.h"

namespace eval {

struct Node {
  Node(std::string fen) : fen_(std::move(fen)) {}

  std::string fen_;
  double result_{};
};

inline bool x_;

struct Param {
  Param(std::string name, int &value, const int initial_value, const int step) : name_(std::move(name)), initial_value_(initial_value), value_(value), step_(step) {
    if (x_)
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

}// namespace eval

namespace {

enum SelectedParams : uint64_t {
  none          = 0,
  psqt          = 1,
  piecevalue    = 1 << 1,
  king          = 1 << 2,
  queen         = 1 << 3,
  rook          = 1 << 4,
  bishop        = 1 << 5,
  knight        = 1 << 6,
  pawn          = 1 << 7,
  passedpawn    = 1 << 8,
  coordination  = 1 << 9,
  centercontrol = 1 << 10,
  color_tempo   = 1 << 11,
  space         = 1 << 12,
  mobility      = 1 << 13,
  attbypawn     = 1 << 14,
  limitadjust   = 1 << 15,
  lazymargin    = 1 << 16,
  weakness      = 1 << 17,
  strength      = 1 << 18
};

std::string emit_code(const std::vector<eval::Param> &params0, const bool hr) {
  std::unordered_map<std::string, std::vector<eval::Param>> params1;

  for (const auto &param : params0)
    params1[param.name_].emplace_back(param);

  fmt::memory_buffer s;

  for (auto &params2 : params1)
  {
    const auto n = params2.second.size();

    fmt::format_to(s, "int Eval::{}", params2.first);

    if (n > 1)
      fmt::format_to(s, "[{}] = { ", n);
    else
      fmt::format_to(s, " = ");

    for (size_t i = 0; i < n; ++i)
    {
      if (hr && n == 64)
      {
        if (i % 8 == 0)
          fmt::format_to(s, "\n ");

        fmt::format_to(s, "{:<{}}", params2.second[i].value_, 4);
      } else
        fmt::format_to(s, "{}", params2.second[i].value_);

      if (n > 1 && i < n - 1)
        fmt::format_to(s, ", ");
    }

    if (n > 1)
      fmt::format_to(s, " }");

    fmt::format_to(s, ";\n");
  }

  return fmt::to_string(s);
}

void print_best_values(double E, const std::vector<eval::Param> &params) {
  auto finished = 0;

  for (std::size_t i = 0; i < params.size(); ++i)
  {
    if (params[i].step_ == 0)
      finished++;

    fmt::print("{}:{}:{}  step:{}\n", i, params[i].name_, params[i].value_, params[i].step_);
  }

  fmt::print("Best E:{:.{}f}  \n", E);
  fmt::print("Finished:{}%\n", finished * 100.0 / params.size());
}


constexpr double bestK() {
  /*
  double smallestE;
  double bestK = -1;
  for (double K = 1.10; K < 1.15; K += 0.01) {
      double _E = E(pgn.all_nodes_, params, K);
      if (bestK < 0 || _E < smallestE) {
        bestK = K;
        smallestE = _E;
      }
      std::cout << "K:" << K << "  _E:" << _E << endl;
  }
  std::cout << "bestK:" << bestK << "smallestE:" << smallestE << endl;
  */
  return 1.12;
}

SelectedParams resolve_params(const std::map<std::string, docopt::value> &args) {
  int result{none};

  for (const auto &elem : args)
  {
    if (elem.second.asBool())
    {
      if (elem.first == "--pawn")
        result |= pawn;
      else if (elem.first == "--knight")
        result |= knight;
      else if (elem.first == "--bishop")
        result |= bishop;
      else if (elem.first == "--rook")
        result |= rook;
      else if (elem.first == "--queen")
        result |= queen;
      else if (elem.first == "--king")
        result |= king;
      else if (elem.first == "--psqt")
        result |= psqt;
      else if (elem.first == "--mobility")
        result |= mobility;
      else if (elem.first == "--passedpawn")
        result |= passedpawn;
      else if (elem.first == "--coordination")
        result |= coordination;
      else if (elem.first == "--strength")
        result |= strength;
      else if (elem.first == "--weakness")
        result |= weakness;
      else if (elem.first == "--tempo")
        result |= tempo;
      else if (elem.first == "--lazy_margin")
        result |= lazymargin;
      else
        fmt::print("Unknown parameter, outdated version.\n");
    }
  }

  return static_cast<SelectedParams>(result);
}

}// namespace

void init_eval(std::vector<eval::Param> &params, const std::map<std::string, docopt::value> &args) {
  auto step = 1;

  const auto current_parameters = resolve_params(args);

  // Check parameters

  eval::x_ = false;

  if (current_parameters & SelectedParams::pawn)
  {
    if (current_parameters & SelectedParams::psqt)
    {
      for (const auto sq : Squares)
      {
        auto istep = sq > 7 && sq < 56 ? step : 0;
        params.emplace_back("pawn_pst_mg", pawn_pst_mg[sq], 0, istep);
        params.emplace_back("pawn_pst_eg", pawn_pst_eg[sq], 0, istep);
      }
    }

    if (current_parameters & SelectedParams::mobility)
    {
      for (auto &v : pawn_isolated_mg)
        params.emplace_back("pawn_isolated_mg", v, 0, step);

      for (auto &v : pawn_isolated_eg)
        params.emplace_back("pawn_isolated_eg", v, 0, step);

      for (auto &v : pawn_behind_mg)
        params.emplace_back("pawn_behind_mg", v, 0, step);

      for (auto &v : pawn_doubled_mg)
        params.emplace_back("pawn_doubled_mg", v, 0, step);

      for (auto &v : pawn_doubled_eg)
        params.emplace_back("pawn_doubled_eg", v, 0, step);
    }

    if (current_parameters & SelectedParams::passedpawn)
    {
      for (auto &v : passed_pawn_mg)
        params.emplace_back("passed_pawn_mg", v, 0, step);

      for (auto &v : passed_pawn_no_us)
        params.emplace_back("passed_pawn_no_us", v, 0, step);

      for (auto &v : passed_pawn_no_them)
        params.emplace_back("passed_pawn_no_them", v, 0, step);

      for (auto &v : passed_pawn_no_attacks)
        params.emplace_back("passed_pawn_no_attacks", v, 0, step);

      for (auto &v : passed_pawn_king_dist_us)
        params.emplace_back("passed_pawn_king_dist_us", v, 0, step);

      for (auto &v : passed_pawn_king_dist_them)
        params.emplace_back("passed_pawn_king_dist_them", v, 0, step);
    }
  }

  if (current_parameters & SelectedParams::knight)
  {
    if (current_parameters & SelectedParams::psqt)
    {
      for (const auto sq : Squares)
      {
        params.emplace_back("knight_pst_mg", knight_pst_mg[sq], 0, step);
        params.emplace_back("knight_pst_eg", knight_pst_eg[sq], 0, step);
      }
    }

    if (current_parameters & SelectedParams::mobility)
    {
      for (auto &v : knight_mob_mg)
        params.emplace_back("knight_mob_mg", v, 0, step);

      for (auto &v : knight_mob_eg)
        params.emplace_back("knight_mob_eg", v, 0, step);

      for (auto &v : knight_mob2_mg)
        params.emplace_back("knight_mob2_mg", v, 0, step);

      for (auto &v : knight_mob2_eg)
        params.emplace_back("knight_mob2_eg", v, 0, step);
    }

    if (current_parameters & SelectedParams::weakness)
      params.emplace_back("knight_in_danger", knight_in_danger, 0, step);

    if (current_parameters & SelectedParams::strength)
      params.emplace_back("knight_attack_king", knight_attack_king, 0, step);
  }

  if (current_parameters & SelectedParams::bishop)
  {
    if (current_parameters & SelectedParams::psqt)
    {
      for (const auto sq : Squares)
      {
        params.emplace_back("bishop_pst_mg", bishop_pst_mg[sq], 0, step);
        params.emplace_back("bishop_pst_eg", bishop_pst_eg[sq], 0, step);
      }
    }

    if (current_parameters & SelectedParams::mobility)
    {
      for (auto &v : bishop_mob_mg)
        params.emplace_back("bishop_mob_mg", v, 0, step);

      for (auto &v : bishop_mob_eg)
        params.emplace_back("bishop_mob_eg", v, 0, step);

      for (auto &v : bishop_mob2_mg)
        params.emplace_back("bishop_mob2_mg", v, 0, step);

      for (auto &v : bishop_mob2_eg)
        params.emplace_back("bishop_mob2_eg", v, 0, step);
    }

    if (current_parameters & SelectedParams::coordination)
    {
      params.emplace_back("bishop_pair_mg", bishop_pair_mg, 0, step);
      params.emplace_back("bishop_pair_eg", bishop_pair_eg, 0, step);
    }

    if (current_parameters & SelectedParams::weakness)
      params.emplace_back("bishop_in_danger", bishop_in_danger, 0, step);

    if (current_parameters & SelectedParams::strength)
      params.emplace_back("bishop_attack_king", bishop_attack_king, 0, step);
  }

  if (current_parameters & SelectedParams::rook)
  {
    if (current_parameters & SelectedParams::psqt)
    {
      for (const auto sq : Squares)
      {
        params.emplace_back("rook_pst_mg", rook_pst_mg[sq], 0, step);
        params.emplace_back("rook_pst_eg", rook_pst_eg[sq], 0, step);
      }
    }

    if (current_parameters & SelectedParams::mobility)
    {
      for (auto &v : rook_mob_mg)
        params.emplace_back("rook_mob_mg", v, 0, step);

      for (auto &v : rook_mob_eg)
        params.emplace_back("rook_mob_eg", v, 0, step);

      params.emplace_back("king_obstructs_rook", king_obstructs_rook, 0, step);
      params.emplace_back("rook_open_file", rook_open_file, 0, step);
    }

    if (current_parameters & SelectedParams::weakness)
      params.emplace_back("rook_in_danger", rook_in_danger, 0, step);

    if (current_parameters & SelectedParams::strength)
      params.emplace_back("rook_attack_king", rook_attack_king, 0, step);
  }

  if (current_parameters & SelectedParams::queen)
  {
    if (current_parameters & SelectedParams::psqt)
    {
      for (const auto sq : Squares)
      {
        params.emplace_back("queen_pst_mg", queen_pst_mg[sq], 0, step);
        params.emplace_back("queen_pst_eg", queen_pst_eg[sq], 0, step);
      }
    }

    if (current_parameters & SelectedParams::mobility)
    {
      for (auto &v : queen_mob_mg)
        params.emplace_back("queen_mob_mg", v, 0, step);

      for (auto &v : queen_mob_eg)
        params.emplace_back("queen_mob_eg", v, 0, step);
    }

    if (current_parameters & SelectedParams::weakness)
      params.emplace_back("queen_in_danger", queen_in_danger, 0, step);

    if (current_parameters & SelectedParams::strength)
      params.emplace_back("queen_attack_king", queen_attack_king, 0, step);
  }

  if (current_parameters & SelectedParams::king)
  {
    if (current_parameters & SelectedParams::psqt)
    {
      for (const auto sq : Squares)
      {
        params.emplace_back("king_pst_mg", king_pst_mg[sq], 0, step);
        params.emplace_back("king_pst_eg", king_pst_eg[sq], 0, step);
      }
    }

    for (auto &v : king_pawn_shelter)
      params.emplace_back("king_pawn_shelter", v, 0, step);

    for (auto &v : king_on_open)
      params.emplace_back("king_on_open", v, 0, step);

    for (auto &v : king_on_half_open)
      params.emplace_back("king_on_half_open", v, 0, step);
  }

  if (current_parameters & SelectedParams::lazymargin)
    params.emplace_back("lazy_margin", lazy_margin, 0, step);

  if (current_parameters & SelectedParams::color_tempo)
    params.emplace_back("tempo", tempo, 0, step);

}

namespace eval {

PGNPlayer::PGNPlayer() : pgn::PGNPlayer(), all_nodes_count_(0) {}

void PGNPlayer::read_pgn_database() {
  PGNFileReader::read_pgn_database();
  print_progress(true);
}

void PGNPlayer::read_san_move() {
  pgn::PGNPlayer::read_san_move();

  all_nodes_count_++;

  if (game_->pos - game_->position_list >= 14 && all_nodes_count_ % 7 == 0)
    current_game_nodes_.emplace_back(game_->get_fen());
}

void PGNPlayer::read_game_termination() {
  pgn::PGNPlayer::read_game_termination();

  for (auto &node : current_game_nodes_)
    node.result_ = result_ == WhiteWin ? 1 : result_ == Draw ? 0.5 : 0;

  all_selected_nodes_.insert(all_selected_nodes_.end(), current_game_nodes_.begin(), current_game_nodes_.end());
  current_game_nodes_.clear();

  print_progress(false);
}

void PGNPlayer::read_comment1() {
  pgn::PGNPlayer::read_comment1();
}

void PGNPlayer::print_progress(const bool force) const {
  if (!force && game_count_ % 100 != 0)
    return;

  fmt::print("game_count_: {} position_count_: {},  all_nodes_.size: {}\n", game_count_, all_nodes_count_, all_selected_nodes_.size());
}

Tune::Tune(Game *game, const std::string_view input, const std::string_view output, const std::map<std::string, docopt::value> &args) : game_(game), score_static_(false) {
  PGNPlayer pgn;
  pgn.read(input.data());

  // Tuning as described in https://chessprogramming.wikispaces.com/Texel%27s+Tuning+Method

  score_static_ = true;

  std::vector<eval::Param> params;

  init_eval(params, args);

  if (score_static_)
    make_quiet(pgn.all_selected_nodes_);

  std::vector<ParamIndexRecord> params_index(params.size());

  for (std::size_t i = 0; i < params.size(); ++i)
    params_index.emplace_back(ParamIndexRecord{i, 0});

  constexpr const auto K = bestK();
  auto bestE             = e(pgn.all_selected_nodes_, params, params_index, K);
  auto improved          = true;

  std::ofstream out(output.data());
  out << fmt::format("Old E:{}\n", bestE);
  out << fmt::format("Old Values:\n{}\n", emit_code(params, true));

  while (improved)
  {
    print_best_values(bestE, params);
    improved = false;

    for (std::size_t i = 0; i < params_index.size(); ++i)
    {
      auto idx    = params_index[i].idx_;
      auto &step  = params[idx].step_;
      auto &value = params[idx].value_;

      if (step == 0)
        continue;

      value += step;

      fmt::print("Tuning prm[{}] {} i:{}  current:{}  trying:{}...\n", idx, params[idx].name_, i, value - step, value);

      auto newE = e(pgn.all_selected_nodes_, params, params_index, K);

      if (newE < bestE)
      {
        params_index[i].improved_ = bestE - newE;
        bestE                     = newE;
        improved                  = true;
        out << "E:" << bestE << "\n";
        out << emit_code(params, true);
      } else if (step > 0)
      {
        step = -step;
        value += 2 * step;

        fmt::print("Tuning prm[{}] {} i:{}  current:{}  trying:{}...\n", idx, params[idx].name_, i, value - step, value);

        newE = e(pgn.all_selected_nodes_, params, params_index, K);

        if (newE < bestE)
        {
          params_index[i].improved_ = bestE - newE;
          bestE                     = newE;
          improved                  = true;
          out << "E:" << bestE << "\n";
          out << emit_code(params, true);
        } else
        {
          params_index[i].improved_ = 0;
          value -= step;
          step = 0;
        }
      } else
      {
        params_index[i].improved_ = 0;
        value -= step;
        step = 0;
      }
    }

    if (improved)
    {
      // std::stable_sort(params_index.begin(), params_index.end());
    }
  }
  print_best_values(bestE, params);
  out << fmt::format("New E:{}\n", bestE);
  out << fmt::format("\nNew:\n{}\n", emit_code(params, false));

  fmt::print("{}\n", emit_code(params, true));
}

double Tune::e(const std::vector<eval::Node> &nodes, const std::vector<eval::Param> &params, const std::vector<eval::ParamIndexRecord> &params_index, double K) {
  double x = 0;

  for (const auto &node : nodes)
  {
    game_->set_fen(node.fen_.c_str());
    x += pow(node.result_ - util::sigmoid(get_score(0), K), 2);
  }
  x /= nodes.size();

  fmt::print("x:{:.{}f}", x, 12);

  for (std::size_t i = 0; i < params_index.size(); ++i)
    if (params[params_index[i].idx_].step_)
      fmt::print(" prm[{}]:{}", i, params[params_index[i].idx_].value_);

  fmt::print("\n");
  return x;
}

void Tune::make_quiet(std::vector<Node> &nodes) {
  for (auto &node : nodes)
  {
    game_->set_fen(node.fen_.c_str());
    pv_length[0] = 0;
    get_quiesce_score(-32768, 32768, true, 0);
    play_pv();
    node.fen_ = game_->get_fen();
  }
}

int Tune::get_score(const int side) {
  const auto score = score_static_ ? Eval::tune(game_, nullptr, -100000, 100000) : get_quiesce_score(-32768, 32768, false, 0);

  return game_->pos->side_to_move == side ? score : -score;
}

int Tune::get_quiesce_score(int alpha, const int beta, const bool store_pv, const int ply) {
  auto score = Eval::tune(game_, nullptr, -100000, 100000);

  if (score >= beta)
    return score;

  auto best_score = score;

  if (best_score > alpha)
    alpha = best_score;

  game_->pos->generate_captures_and_promotions(this);

  while (auto *const move_data = game_->pos->next_move())
  {
    if (!is_promotion(move_data->move))
    {
      if (move_data->score < 0)
        break;
    }

    if (make_move(move_data->move, ply))
    {
      score = -get_quiesce_score(-beta, -alpha, store_pv, ply + 1);

      game_->unmake_move();

      if (score > best_score)
      {
        best_score = score;

        if (best_score > alpha)
        {
          if (score >= beta)
            break;

          if (store_pv)
            update_pv(move_data->move, best_score, ply);

          alpha = best_score;
        }
      }
    }
  }
  return best_score;
}

bool Tune::make_move(const uint32_t m, int ply) {
  if (game_->make_move(m, true, true))
  {
    ++ply;
    pv_length[ply] = ply;
    return true;
  }
  return false;
}

void Tune::unmake_move() const {
  game_->unmake_move();
}

void Tune::play_pv() {
  for (auto i = 0; i < pv_length[0]; ++i)
    game_->make_move(pv[0][i].move, false, true);
}

void Tune::update_pv(const uint32_t move, const int score, const int ply) {
  assert(ply < 128);
  assert(pv_length[ply] < 128);
  auto *entry = &pv[ply][ply];

  entry->score = score;
  entry->move  = move;
  // entry->eval = game_->pos->eval_score;

  pv_length[ply] = pv_length[ply + 1];

  for (auto i = ply + 1; i < pv_length[ply]; ++i)
    pv[ply][i] = pv[ply + 1][i];
}

void Tune::sort_move(MoveData &move_data) {
  const auto m = move_data.move;

  if (is_queen_promotion(m))
    move_data.score = 890000;
  else if (is_capture(m))
  {
    const auto value_captured = piece_value(move_captured(m));
    auto value_piece          = piece_value(move_piece(m));

    if (value_piece == 0)
      value_piece = 1800;

    if (value_piece <= value_captured)
      move_data.score = 300000 + value_captured * 20 - value_piece;
    else if (game_->board.see_move(m) >= 0)// TODO : create See on the fly?
      move_data.score = 160000 + value_captured * 20 - value_piece;
    else
      move_data.score = -100000 + value_captured * 20 - value_piece;
  } else
    exit(0);
}

}// namespace eval
