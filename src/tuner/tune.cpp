// Copyright (C) 2008-2016 Gunnar Harms (Bobcat author)
// Copyright (C) 2017      FireFather (Tomcat author)
// Copyright (C) 2020-2025 Rudy Alex Kohn
// See end of file for extended copyright information.

#include <cassert>
#include <string>
#include <fstream>
#include <utility>
#include <unordered_map>

#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include "tune.hpp"
#include "map.h"

#include <felis/board.hpp>
#include <felis/eval.hpp>
#include <felis/parameters.hpp>
#include <felis/util.hpp>
#include <felis/tpool.hpp>

namespace eval {

struct Node final {
  Node(std::string fen) : fen_(std::move(fen)) {}

  std::string fen_;
  double result_{};
};

inline bool x_;

struct Param final {
  Param(std::string name, Score& value, const Score initialValue, const int step, const int stages = 2) : name_(std::move(name)), initial_value_(initialValue), value_(value), step_(step), stages_(stages) {
    if (x_)
      value = initialValue;
  }

  std::string name_;
  Score initial_value_;
  Score& value_;
  int step_;
  int stages_;
};

struct ParamIndexRecord final {
  size_t idx_{};
  double improved_{};
};

inline bool operator<(const ParamIndexRecord& lhs, const ParamIndexRecord& rhs) {
  return lhs.improved_ >= rhs.improved_;
}

}   // namespace eval

namespace {

auto console   = spdlog::stdout_color_mt("tuner");
auto errLogger = spdlog::stderr_color_mt("stderr");
std::shared_ptr<spdlog::logger> fileLogger;

enum SelectedParams : std::uint64_t {
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
std::string emitCode(std::vector<eval::Param>& params0) {
  std::unordered_map<std::string, std::vector<eval::Param>> params1;

  for (const auto& param : params0)
    params1[param.name_].emplace_back(param);

  fmt::memory_buffer s;
  auto inserter = std::back_inserter(s);

  for (auto& params2 : params1) {
    const auto n = params2.second.size();

    if (n > 1)
      fmt::format_to(inserter, "inline std::array<int, {}> {} {{", n, params2.first);
    else
      fmt::format_to(inserter, "inline int {} = ", params2.first);

    for (size_t i = 0; i < n; ++i) {
      const Score s = params2.second[i].value_;
      if (Hr && n == 64) {
        if (i % 8 == 0)
          fmt::format_to(inserter, "\n ");
      }
      fmt::format_to(inserter, "m:{} e:{}", s.mg(), s.eg());

      if (n > 1 && i < n - 1)
        fmt::format_to(inserter, ", ");
    }

    if (n > 1)
      fmt::format_to(inserter, " }}");

    fmt::format_to(inserter, ";\n");
  }

  return fmt::to_string(s);
}

void printBestValues(const double e, const std::vector<eval::Param>& params) {
  auto finished = 0;

  for (std::size_t i = 0; i < params.size(); ++i) {
    if (params[i].step_ == 0)
      finished++;
    const Score s = params[i].value_;
    console->info("{}:mg={},eg={}:{}  step:{}\n", i, params[i].name_, s.mg(), s.eg(), params[i].step_);
  }

  console->info("Best E:{}", e);
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

void initEval(std::vector<eval::Param>& params, const ParserSettings* settings) {
  auto step = 1;

  eval::x_ = false;

  if (settings->pawn) {
    console->info("Pawn tuning active");
    if (settings->psqt) {
      console->info("Pawn psqt tuning active");
      for (const auto sq : Squares) {
        auto istep = sq > 7 && sq < 56 ? step : 0;
        params.emplace_back("pawn_pst", params::pst<PAWN>(sq), 0, istep);
      }
    }

    if (settings->mobility) {
      console->info("Pawn mobility tuning active");
      for (auto& v : params::pawn_isolated)
        params.emplace_back("pawn_isolated", v, 0, step);

      for (auto& v : params::pawn_behind)
        params.emplace_back("pawn_behind", v, 0, step);

      for (auto& v : params::pawn_doubled)
        params.emplace_back("pawn_doubled", v, 0, step);
    }

    if (settings->passed_pawn) {
      for (auto& v : params::passed_pawn)
        params.emplace_back("passed_pawn", v, 0, step);

      for (auto& v : params::passed_pawn_no_us)
        params.emplace_back("passed_pawn_no_us", v, 0, step);

      for (auto& v : params::passed_pawn_no_them)
        params.emplace_back("passed_pawn_no_them", v, 0, step);

      for (auto& v : params::passed_pawn_no_attacks)
        params.emplace_back("passed_pawn_no_attacks", v, 0, step);

      for (auto& v : params::passed_pawn_king_dist_us)
        params.emplace_back("passed_pawn_king_dist_us", v, 0, step);

      for (auto& v : params::passed_pawn_king_dist_them)
        params.emplace_back("passed_pawn_king_dist_them", v, 0, step);
    }
  }

  if (settings->knight) {
    if (settings->psqt) {
      for (const auto sq : Squares) {
        params.emplace_back("knight_pst", params::pst<KNIGHT>(sq), 0, step);
      }
    }

    if (settings->mobility) {
      for (auto& v : params::knight_mob)
        params.emplace_back("knight_mob", v, 0, step);

      for (auto& v : params::knight_mob2)
        params.emplace_back("knight_mob2", v, 0, step);

      for (auto& v : params::knight_mob2)
        params.emplace_back("knight_mob2", v, 0, step);
    }

    // if (settings->weakness)
    //   params.emplace_back("knight_in_danger", params::piece_in_danger[Knight], 0, step);

    // if (settings->strength)
    //   params.emplace_back("knight_attack_king", params::attacks_on_king[Knight], 0, step);
  }

  if (settings->bishop) {
    if (settings->psqt) {
      for (const auto sq : Squares)
        params.emplace_back("bishop_pst", params::pst<BISHOP>(sq), 0, step);
    }

    if (settings->mobility) {
      for (auto& v : params::bishop_mob)
        params.emplace_back("bishop_mob", v, 0, step);

      for (auto& v : params::bishop_mob2)
        params.emplace_back("bishop_mob2", v, 0, step);
    }

    if (settings->coordination) {
      params.emplace_back("bishop_pair", params::bishop_pair, 0, step);
    }

    // if (settings->weakness)
    //   params.emplace_back("bishop_in_danger", params::piece_in_danger[Bishop], 0, step);

    // if (settings->strength)
    //   params.emplace_back("attacks_on_king[Bishop]", params::attacks_on_king[Bishop], 0, step);
  }

  if (settings->rook) {
    if (settings->psqt) {
      for (const auto sq : Squares) {
        params.emplace_back("rook_pst", params::pst<ROOK>(sq), 0, step);
      }
    }

    if (settings->mobility) {
      for (auto& v : params::rook_mob)
        params.emplace_back("rook_mob", v, 0, step);

      params.emplace_back("king_obstructs_rook", params::king_obstructs_rook, 0, step);
    }

    // params.emplace_back("rook_open_file", params::rook_open_file, 0, step);

    // if (settings->weakness)
    //   params.emplace_back("rook_in_danger", params::piece_in_danger[Rook], 0, step);

    // if (settings->strength)
    //   params.emplace_back("attacks_on_king[Rook]", params::attacks_on_king[Rook], 0, step);
  }

  if (settings->queen) {
    if (settings->psqt) {
      for (const auto sq : Squares)
        params.emplace_back("queen_pst", params::pst<QUEEN>(sq), 0, step);
    }

    if (settings->mobility) {
      for (auto& v : params::queen_mob)
        params.emplace_back("queen_mob", v, 0, step);
    }

    // if (settings->weakness)
    //   params.emplace_back("queen_in_danger", params::piece_in_danger[Queen], 0, step);

    // if (settings->strength)
    //   params.emplace_back("attacks_on_king[Queen]", params::attacks_on_king[Queen], 0, step);
  }

  if (settings->king) {
    if (settings->psqt) {
      for (const auto sq : Squares)
        params.emplace_back("king_pst", params::pst<KING>(sq), 0, step);
    }

    for (auto& v : params::king_pawn_shelter)
      params.emplace_back("king_pawn_shelter", v, 0, step);

    for (auto& v : params::king_on_open)
      params.emplace_back("king_on_open", v, 0, step);

    for (auto& v : params::king_on_half_open)
      params.emplace_back("king_on_half_open", v, 0, step);
  }

  // if (settings->lazy_margin)
  //   params.emplace_back("lazy_margin", params::lazy_margin, 0, step);

  // if (settings->tempo)
  //   params.emplace_back("tempo", params::tempo, 0, step);
}

}   // namespace

namespace eval {

PGNPlayer::PGNPlayer() : pgn::PGNPlayer() {}

void PGNPlayer::read_pgn_database() {
  PGNFileReader::read_pgn_database();
  printProgress(true);
}

void PGNPlayer::read_san_move() {
  pgn::PGNPlayer::read_san_move();

  all_nodes_count_++;

  if (b->half_move_count() >= 14 && all_nodes_count_ % 7 == 0)
    current_game_nodes_.emplace_back(b->fen());
}

void PGNPlayer::read_game_termination() {
  pgn::PGNPlayer::read_game_termination();

  for (auto& node : current_game_nodes_)
    node.result_ = result_ == WhiteWin ? 1 : result_ == Draw ? 0.5 : 0;

  all_selected_nodes_.insert(all_selected_nodes_.end(), current_game_nodes_.begin(), current_game_nodes_.end());
  current_game_nodes_.clear();

  printProgress(false);
}

void PGNPlayer::read_comment1() {
  pgn::PGNPlayer::read_comment1();
}

void PGNPlayer::printProgress(const bool force) const {
  if (!force && game_count_ % 100 != 0)
    return;

  fmt::print("game_count_: {} position_count_: {},  all_nodes_.size: {}\n", game_count_, all_nodes_count_, all_selected_nodes_.size());
}

Tune::Tune(std::unique_ptr<Board> board, const ParserSettings* settings) : b(std::move(board)), score_static_(false) {
  PGNPlayer pgn;
  pgn.read(settings->file_name);

  // Tuning as described in https://www.chessprogramming.org/Texel%27s_Tuning_Method

  score_static_ = true;

  std::vector<Param> params;

  initEval(params, settings);

  if (score_static_)
    makeQuiet(pgn.all_selected_nodes_);

  std::vector<ParamIndexRecord> params_index;

  for (std::size_t i = 0; i < params.size(); ++i)
    params_index.emplace_back(i, 0);

  constexpr i32 max_log_file_size = 1048576 * 5;
  constexpr i32 max_log_files     = 3;
  constexpr r64 K                 = bestK();
  r64 bestE                       = e(pgn.all_selected_nodes_, params, params_index, K);
  bool improved                   = true;
  fileLogger                      = spdlog::rotating_logger_mt("fileLogger", "logs/tuner.txt", max_log_file_size, max_log_files);

  fileLogger->info("Tuner session started.");

  std::ofstream out(fmt::format("{}{}", settings->file_name, ".txt"));
  out << fmt::format("Old E:{}\n", bestE);
  auto t = emitCode<true>(params);
  out << fmt::format("Old Values:\n{}\n", t);

  // 0 == mg, 1 == eg
  auto stage = 0;
  Score original;

  while (improved) {
    printBestValues(bestE, params);
    improved = false;

    for (std::size_t i = 0; i < params_index.size(); ++i) {
      auto idx    = params_index[i].idx_;
      auto& step  = params[idx].step_;
      auto& value = params[idx].value_;
      do {

        if (step == 0)
          continue;

        // cycle through mg & eg

        original = value;

        if (stage++ == 0)
          value += step;
        else
          value = Score(value.mg(), value.eg() + step);

        fmt::print("Tuning prm[{}] {} i:{}  current:{}{}  trying:{}/{}...\n", idx, params[idx].name_, i, original.mg(), original.eg(), value.mg(), value.eg());

        auto newE = e(pgn.all_selected_nodes_, params, params_index, K);

        if (newE < bestE) {
          params_index[i].improved_ = bestE - newE;
          bestE                     = newE;
          improved                  = true;
          out << "E:" << bestE << "\n";
          out << emitCode<true>(params);
        } else if (step > 0) {
          step = -step;
          value += 2 * step;

          Score s = value - step;
          fmt::print("Tuning prm[{}] {} i:{}  current:{}{}  trying:{}/{}...\n", idx, params[idx].name_, i, s.mg(), s.eg(), value.mg(), value.eg());

          newE = e(pgn.all_selected_nodes_, params, params_index, K);

          if (newE < bestE) {
            params_index[i].improved_ = bestE - newE;
            bestE                     = newE;
            improved                  = true;
            out << "E:" << bestE << "\n";
            out << emitCode<true>(params);
          } else {
            params_index[i].improved_ = 0;
            value                     = original;
            step                      = 0;
          }
        } else {
          params_index[i].improved_ = 0;
          value                     = original;
          step                      = 0;
        }

      } while (stage <= params[idx].stages_);
    }

    if (improved) {
      // std::stable_sort(params_index.begin(), params_index.end());
    }
  }
  printBestValues(bestE, params);
  out << fmt::format("New E:{}\n", bestE);
  out << fmt::format("\nNew:\n{}\n", emitCode<false>(params));

  fmt::print("{}\n", emitCode<true>(params));
}

double Tune::e(const std::vector<Node>& nodes, const std::vector<Param>& params, const std::vector<ParamIndexRecord>& paramsIndex, const double k) {
  auto x = 0.0;

  for (const auto& node : nodes) {
    b->set_fen(node.fen_, pool.main());
    const auto z = node.result_ - util::sigmoid(score(WHITE), k);
    x += z * z;
  }

  x /= nodes.empty() ? 1.0 : static_cast<double>(nodes.size());

  fmt::memory_buffer s;
  auto inserter = std::back_inserter(s);

  fmt::format_to(inserter, "x:{:.{}f}", x, 12);

  for (std::size_t i = 0; i < paramsIndex.size(); ++i)
    if (params[paramsIndex[i].idx_].step_) {
      auto v = params[paramsIndex[i].idx_].value_;
      fmt::format_to(inserter, " prm[{}]:{}/{}\n", i, v.mg(), v.eg());
    }

  console->info("{}\n\n", fmt::to_string(s));

  return x;
}

void Tune::makeQuiet(std::vector<Node>& nodes) {
  auto* t = pool.main();
  for (auto& node : nodes) {
    b->set_fen(node.fen_, t);
    t->pv_length[0] = 0;
    // auto q          = quiesceScore(-32768, 32768, true, 0);
    playPv();
    node.fen_ = b->fen();
  }
}

int Tune::score(const Color c) const {
  const auto score = score_static_ ? Eval::tune(b.get(), 0, -100000, 100000) : quiesceScore(-32768, 32768, false, 0);
  return b->side_to_move() == c ? score : -score;
}

int Tune::quiesceScore(int alpha, const int beta, const bool storePv, const int ply) const {
  auto score = Eval::tune(b.get(), 0, -100000, 100000);

  if (score >= beta)
    return score;

  auto bestScore = score;

  if (bestScore > alpha)
    alpha = bestScore;

  auto mg = Moves<true>(b.get());

  mg.generate_captures_and_promotions();

  // b->pos->generate_captures_and_promotions(this);

  while (const auto* const moveData = mg.next_move()) {
    if (!is_promotion(moveData->move) && moveData->score < 0)
      break;

    if (makeMove(moveData->move, ply)) {
      score = -quiesceScore(-beta, -alpha, storePv, ply + 1);

      b->unmake_move();

      if (score > bestScore) {
        bestScore = score;

        if (bestScore > alpha) {
          if (score >= beta)
            break;

          if (storePv)
            updatePv(moveData->move, bestScore, ply);

          alpha = bestScore;
        }
      }
    }
  }
  return bestScore;
}

bool Tune::makeMove(Move m, int ply) const {
  if (!b->make_move(m, true, true))
    return false;

  ++ply;
  b->my_thread()->pv_length[ply] = ply;
  return true;
}

void Tune::unmakeMove() const {
  b->unmake_move();
}

void Tune::playPv() const {
  auto* t = b->my_thread();
  for (auto i = 0; i < t->pv_length[0]; ++i)
    b->make_move(t->pv[0][i].move, false, true);
}

void Tune::updatePv(Move m, int score, int ply) const {
  auto* t = b->my_thread();
  assert(ply < MAXDEPTH);
  assert(t->pv_length[ply] < MAXDEPTH);
  auto* entry = &t->pv[ply][ply];

  entry->score = score;
  entry->move  = m;
  // entry->eval = game_->pos->eval_score;

  const auto nextPly = ply + 1;

  t->pv_length[ply] = t->pv_length[nextPly];

  for (auto i = nextPly; i < t->pv_length[ply]; ++i)
    t->pv[ply][i] = t->pv[nextPly][i];
}

}   // namespace eval

// Feliscatus, a UCI chess playing engine derived from Tomcat 1.0 (Bobcat 8.0)
// Copyright (C) 2008-2016 Gunnar Harms (Bobcat author)
// Copyright (C) 2017      FireFather (Tomcat author)
// Copyright (C) 2020-2025 Rudy Alex Kohn
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