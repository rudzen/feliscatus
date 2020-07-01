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

#include <cassert>
#include <string>
#include <fstream>
#include <unordered_map>
#include <utility>

#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include "tune.h"
#include "../src/board.h"
#include "../src/eval.h"
#include "../src/parameters.h"
#include "../src/util.h"
#include "file_resolver.h"

namespace eval {

struct Node final {
  Node(std::string fen) : fen_(std::move(fen)) {}

  std::string fen_;
  double result_{};
};

inline bool x_;

struct Param final {
  Param(std::string name, Score &value, const Score initial_value, const int step, const int stages = 2) : name_(std::move(name)), initial_value_(initial_value), value_(value), step_(step), stages_(stages) {
    if (x_)
      value = initial_value;
  }

  std::string name_;
  Score initial_value_;
  Score &value_;
  int step_;
  int stages_;
};

struct ParamIndexRecord final {
  size_t idx_{};
  double improved_{};
};

inline bool operator<(const ParamIndexRecord &lhs, const ParamIndexRecord &rhs) {
  return lhs.improved_ >= rhs.improved_;
}

}// namespace eval

namespace {

auto console = spdlog::stdout_color_mt("tuner");
auto err_logger = spdlog::stderr_color_mt("stderr");
std::shared_ptr<spdlog::logger> file_logger;

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

template<bool Hr>
std::string emit_code(const std::vector<eval::Param> &params0) {

  std::unordered_map<std::string, std::vector<eval::Param>> params1;

  for (const auto &param : params0)
    params1[param.name_].emplace_back(param);

  fmt::memory_buffer s;

  for (auto &params2 : params1)
  {
    const auto n = params2.second.size();

    if (n > 1)
      format_to(s, "inline std::array<int, {}> {} {{", n, params2.first);
    else
      format_to(s, "inline int {} = ", params2.first);

    for (size_t i = 0; i < n; ++i)
    {
      if (Hr && n == 64)
      {
        if (i % 8 == 0)
          format_to(s, "\n ");

        format_to(s, "{}", params2.second[i].value_);
      } else
        format_to(s, "{}", params2.second[i].value_);

      if (n > 1 && i < n - 1)
        format_to(s, ", ");
    }

    if (n > 1)
      format_to(s, " }}");

    format_to(s, ";\n");
  }

  return fmt::to_string(s);
}

void print_best_values(const double E, const std::vector<eval::Param> &params) {
  auto finished = 0;

  for (std::size_t i = 0; i < params.size(); ++i)
  {
    if (params[i].step_ == 0)
      finished++;
    console->info("{}:{}:{}  step:{}\n", i, params[i].name_, params[i].value_, params[i].step_);
  }

  console->info("Best E:{}", E);
  console->info("Finished:{} %", finished == 0 ? 100.0 : finished * 100.0 / params.size());
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


void init_eval(std::vector<eval::Param> &params, const ParserSettings *settings) {
  auto step = 1;

  eval::x_ = false;

  if (settings->pawn)
  {
    console->info("Pawn tuning active");
    if (settings->psqt)
    {
      console->info("Pawn psqt tuning active");
      for (const auto sq : Squares)
      {
        auto istep = sq > 7 && sq < 56 ? step : 0;
        params.emplace_back("pawn_pst", pawn_pst[sq], 0, istep);
      }
    }

    if (settings->mobility)
    {
      console->info("Pawn mobility tuning active");
      for (auto &v : pawn_isolated)
        params.emplace_back("pawn_isolated", v, 0, step);

      for (auto &v : pawn_behind)
        params.emplace_back("pawn_behind", v, 0, step);

      for (auto &v : pawn_doubled)
        params.emplace_back("pawn_doubled", v, 0, step);
    }

    if (settings->passed_pawn)
    {
      for (auto &v : passed_pawn)
        params.emplace_back("passed_pawn", v, 0, step);

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

  if (settings->knight)
  {
    if (settings->psqt)
    {
      for (const auto sq : Squares)
      {
        params.emplace_back("knight_pst", knight_pst[sq], 0, step);
      }
    }

    if (settings->mobility)
    {
      for (auto &v : knight_mob)
        params.emplace_back("knight_mob", v, 0, step);

      for (auto &v : knight_mob2)
        params.emplace_back("knight_mob2", v, 0, step);

      for (auto &v : knight_mob2)
        params.emplace_back("knight_mob2", v, 0, step);
    }

    // if (settings->weakness)
    //   params.emplace_back("knight_in_danger", piece_in_danger[Knight], 0, step);

    // if (settings->strength)
    //   params.emplace_back("knight_attack_king", attacks_on_king[Knight], 0, step);
  }

  if (settings->bishop)
  {
    if (settings->psqt)
    {
      for (const auto sq : Squares)
        params.emplace_back("bishop_pst", bishop_pst[sq], 0, step);
    }

    if (settings->mobility)
    {
      for (auto &v : bishop_mob)
        params.emplace_back("bishop_mob", v, 0, step);

      for (auto &v : bishop_mob2)
        params.emplace_back("bishop_mob2", v, 0, step);
    }

    if (settings->coordination)
    {
      params.emplace_back("bishop_pair", bishop_pair, 0, step);
    }

    // if (settings->weakness)
    //   params.emplace_back("bishop_in_danger", piece_in_danger[Bishop], 0, step);

    // if (settings->strength)
    //   params.emplace_back("attacks_on_king[Bishop]", attacks_on_king[Bishop], 0, step);
  }

  if (settings->rook)
  {
    if (settings->psqt)
    {
      for (const auto sq : Squares)
      {
        params.emplace_back("rook_pst", rook_pst[sq], 0, step);
      }
    }

    if (settings->mobility)
    {
      for (auto &v : rook_mob)
        params.emplace_back("rook_mob", v, 0, step);

      params.emplace_back("king_obstructs_rook", king_obstructs_rook, 0, step);
    }

    // params.emplace_back("rook_open_file", rook_open_file, 0, step);

    // if (settings->weakness)
    //   params.emplace_back("rook_in_danger", piece_in_danger[Rook], 0, step);

    // if (settings->strength)
    //   params.emplace_back("attacks_on_king[Rook]", attacks_on_king[Rook], 0, step);
  }

  if (settings->queen)
  {
    if (settings->psqt)
    {
      for (const auto sq : Squares)
        params.emplace_back("queen_pst", queen_pst[sq], 0, step);
    }

    if (settings->mobility)
    {
      for (auto &v : queen_mob)
        params.emplace_back("queen_mob", v, 0, step);
    }

    // if (settings->weakness)
    //   params.emplace_back("queen_in_danger", piece_in_danger[Queen], 0, step);

    // if (settings->strength)
    //   params.emplace_back("attacks_on_king[Queen]", attacks_on_king[Queen], 0, step);
  }

  if (settings->king)
  {
    if (settings->psqt)
    {
      for (const auto sq : Squares)
        params.emplace_back("king_pst", king_pst[sq], 0, step);
    }

    for (auto &v : king_pawn_shelter)
      params.emplace_back("king_pawn_shelter", v, 0, step);

    for (auto &v : king_on_open)
      params.emplace_back("king_on_open", v, 0, step);

    for (auto &v : king_on_half_open)
      params.emplace_back("king_on_half_open", v, 0, step);
  }

  // if (settings->lazy_margin)
  //   params.emplace_back("lazy_margin", lazy_margin, 0, step);

  // if (settings->tempo)
  //   params.emplace_back("tempo", tempo, 0, step);
}

}// namespace


namespace eval {

PGNPlayer::PGNPlayer() : pgn::PGNPlayer() {}

void PGNPlayer::read_pgn_database() {
  PGNFileReader::read_pgn_database();
  print_progress(true);
}

void PGNPlayer::read_san_move() {
  pgn::PGNPlayer::read_san_move();

  all_nodes_count_++;

  if (board_->half_move_count() >= 14 && all_nodes_count_ % 7 == 0)
    current_game_nodes_.emplace_back(board_->get_fen());
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

Tune::Tune(std::unique_ptr<Board> board, const ParserSettings *settings) : board_(std::move(board)), score_static_(false) {
  PGNPlayer pgn;
  pgn.read(settings->file_name);

  // Tuning as described in https://chessprogramming.wikispaces.com/Texel%27s+Tuning+Method

  score_static_ = true;

  std::vector<Param> params;

  init_eval(params, settings);

  if (score_static_)
    make_quiet(pgn.all_selected_nodes_);

  std::vector<ParamIndexRecord> params_index;

  for (std::size_t i = 0; i < params.size(); ++i)
    params_index.emplace_back(i, 0);

  constexpr auto max_log_file_size = 1048576 * 5;
  constexpr auto max_log_files     = 3;
  constexpr auto K                 = bestK();
  auto bestE                       = e(pgn.all_selected_nodes_, params, params_index, K);
  auto improved                    = true;
  file_logger = spdlog::rotating_logger_mt("file_logger", "logs/tuner.txt", max_log_file_size, max_log_files);

  file_logger->info("Tuner session started.");

  std::ofstream out(fmt::format("{}{}", settings->file_name, ".txt"));
  out << fmt::format("Old E:{}\n", bestE);
  out << fmt::format("Old Values:\n{}\n", emit_code<true>(params));

  // 0 == mg, 1 == eg
  auto stage = 0;
  Score original = ZeroScor;

  while (improved)
  {
    print_best_values(bestE, params);
    improved = false;

    for (std::size_t i = 0; i < params_index.size(); ++i)
    {
      auto idx    = params_index[i].idx_;
      auto &step  = params[idx].step_;
      auto &value = params[idx].value_;
      do {

        if (step == 0)
          continue;

        // cycle through mg & eg

        original = value;

        if (stage++ == 0)
          value += step;
        else
          value = Score(value.mg(), value.eg() + step);

        fmt::print("Tuning prm[{}] {} i:{}  current:{}  trying:{}...\n", idx, params[idx].name_, i, original, value);

        auto new_e = e(pgn.all_selected_nodes_, params, params_index, K);

        if (new_e < bestE)
        {
          params_index[i].improved_ = bestE - new_e;
          bestE                     = new_e;
          improved                  = true;
          out << "E:" << bestE << "\n";
          out << emit_code<true>(params);
        } else if (step > 0)
        {
          step = -step;
          value += 2 * step;

          fmt::print("Tuning prm[{}] {} i:{}  current:{}  trying:{}...\n", idx, params[idx].name_, i, value - step, value);

          new_e = e(pgn.all_selected_nodes_, params, params_index, K);

          if (new_e < bestE)
          {
            params_index[i].improved_ = bestE - new_e;
            bestE                     = new_e;
            improved                  = true;
            out << "E:" << bestE << "\n";
            out << emit_code<true>(params);
          } else
          {
            params_index[i].improved_ = 0;
            value                     = original;
            step                      = 0;
          }
        } else
        {
          params_index[i].improved_ = 0;
          value                     = original;
          step                      = 0;
        }

      } while(stage <= params[idx].stages_);
    }

    if (improved)
    {
      // std::stable_sort(params_index.begin(), params_index.end());
    }
  }
  print_best_values(bestE, params);
  out << fmt::format("New E:{}\n", bestE);
  out << fmt::format("\nNew:\n{}\n", emit_code<false>(params));

  fmt::print("{}\n", emit_code<true>(params));
}

double Tune::e(const std::vector<Node> &nodes, const std::vector<Param> &params, const std::vector<ParamIndexRecord> &params_index, const double K) {
  auto x = 0.0;

  for (const auto &node : nodes)
  {
    board_->set_fen(node.fen_);
    x += std::pow(node.result_ - util::sigmoid(get_score(WHITE), K), 2);
  }

  x /= nodes.empty() ? 1.0 : static_cast<double>(nodes.size());

  fmt::memory_buffer s;

  //fmt::print("x:{}", x);
  format_to(s, "x:{:.{}f}", x, 12);

  for (std::size_t i = 0; i < params_index.size(); ++i)
    if (params[params_index[i].idx_].step_)
      format_to(s, " prm[{}]:{}\n", i, params[params_index[i].idx_].value_);

  format_to(s, "\n");

  console->info("{}\n", fmt::to_string(s));

  return x;
}

void Tune::make_quiet(std::vector<Node> &nodes) {
  for (auto &node : nodes)
  {
    board_->set_fen(node.fen_.c_str());
    pv_length[0] = 0;
    get_quiesce_score(-32768, 32768, true, 0);
    play_pv();
    node.fen_ = board_->get_fen();
  }
}

int Tune::get_score(const Color side) {
  const auto score = score_static_ ? Eval::tune(board_.get(), 0, -100000, 100000) : get_quiesce_score(-32768, 32768, false, 0);
  return board_->pos->side_to_move == side ? score : -score;
}

int Tune::get_quiesce_score(int alpha, const int beta, const bool store_pv, const int ply) {
  auto score = Eval::tune(board_.get(), 0, -100000, 100000);

  if (score >= beta)
    return score;

  auto best_score = score;

  if (best_score > alpha)
    alpha = best_score;

  board_->pos->generate_captures_and_promotions(this);

  while (auto *const move_data = board_->pos->next_move())
  {
    if (!is_promotion(move_data->move) && move_data->score < 0)
      break;

    if (make_move(move_data->move, ply))
    {
      score = -get_quiesce_score(-beta, -alpha, store_pv, ply + 1);

      board_->unmake_move();

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

bool Tune::make_move(const Move m, int ply) {
  if (!board_->make_move(m, true, true))
    return false;

  ++ply;
  pv_length[ply] = ply;
  return true;
}

void Tune::unmake_move() const {
  board_->unmake_move();
}

void Tune::play_pv() {
  for (auto i = 0; i < pv_length[0]; ++i)
    board_->make_move(pv[0][i].move, false, true);
}

void Tune::update_pv(const Move move, const int score, const int ply) {
  assert(ply < MAXDEPTH);
  assert(pv_length[ply] < MAXDEPTH);
  auto *entry = &pv[ply][ply];

  entry->score = score;
  entry->move  = move;
  // entry->eval = game_->pos->eval_score;

  const auto next_ply = ply + 1;

  pv_length[ply] = pv_length[next_ply];

  for (auto i = next_ply; i < pv_length[ply]; ++i)
    pv[ply][i] = pv[next_ply][i];
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
    else if (board_->see_move(m) >= 0)
      move_data.score = 160000 + value_captured * 20 - value_piece;
    else
      move_data.score = -100000 + value_captured * 20 - value_piece;
  } else
    exit(0);
}

}// namespace eval
