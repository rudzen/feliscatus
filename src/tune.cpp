#include <fmt/format.h>
#include <fstream>
#include <unordered_map>
#include "tune.h"
#include "game.h"
#include "eval.h"
#include "parameters.h"

namespace {

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

}// namespace

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

Tune::Tune(Game *game)
  : game_(game), score_static_(false) {
  PGNPlayer pgn;
  pgn.read(R"(d:\tomcat\x64\result.pgn)");

  // Tuning as described in https://chessprogramming.wikispaces.com/Texel%27s+Tuning+Method

  score_static_ = true;

  std::vector<Param> params;

  init_eval(params);

  if (score_static_)
    make_quiet(pgn.all_selected_nodes_);

  std::vector<ParamIndexRecord> params_index(params.size());

  for (std::size_t i = 0; i < params.size(); ++i)
    params_index.emplace_back(ParamIndexRecord{i, 0});

  constexpr const auto K      = bestK();
  auto bestE  = e(pgn.all_selected_nodes_, params, params_index, K);
  auto improved = true;

  std::ofstream out(R"(d:\tomcat\x64\tune.txt)");
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

void Tune::init_eval(std::vector<Param> &params) {
  auto step = 1;
  x_        = false;
  /*
                  for (auto i = 0; i < 9; ++i)
                          {
                          params.emplace_back(Param("knight_mob_mg", eval_->knight_mob_mg[i], 0, step));
                          params.emplace_back(Param("knight_mob_eg", eval_->knight_mob_eg[i], 0, step));
                          }

                  for (auto i = 0; i < 14; ++i)
                          {
                          params.emplace_back(Param("bishop_mob_mg", eval_->bishop_mob_mg[i], 0, step));
                          params.emplace_back(Param("bishop_mob_eg", eval_->bishop_mob_eg[i], 0, step));
                          }

                  for (auto i = 0; i < 15; ++i)
                          {
                          params.emplace_back(Param("rook_mob_mg", eval_->rook_mob_mg[i], 0, step));
                          params.emplace_back(Param("rook_mob_eg", eval_->rook_mob_eg[i], 0, step));
                          }

                  for (auto i = 0; i < 28; ++i)
                          {
                          params.emplace_back(Param("queen_mob_mg", eval_->queen_mob_mg[i], 0, step));
                          params.emplace_back(Param("queen_mob_eg", eval_->queen_mob_eg[i], 0, step));
                          }

                  for (auto i = 0; i < 9; ++i)
                          {
                          params.emplace_back(Param("knight_mob2_mg", eval_->knight_mob2_mg[i], 0, step));
                          params.emplace_back(Param("knight_mob2_eg", eval_->knight_mob2_eg[i], 0, step));
                          }

                  for (auto i = 0; i < 14; ++i)
                          {
                          params.emplace_back(Param("bishop_mob2_mg", eval_->bishop_mob2_mg[i], 0, step));
                          params.emplace_back(Param("bishop_mob2_eg", eval_->bishop_mob2_eg[i], 0, step));
                          }
  */
  //----------------------------------
  for (const auto i : Squares)
  {
    auto istep = i > 7 && i < 56 ? step : 0;
    params.emplace_back("pawn_pst_mg", pawn_pst_mg[i], 0, istep);
    params.emplace_back("pawn_pst_eg", pawn_pst_eg[i], 0, istep);
  }

  for (const auto i : Squares)
  {
    params.emplace_back("knight_pst_mg", knight_pst_mg[i], 0, step);
    params.emplace_back("knight_pst_eg", knight_pst_eg[i], 0, step);
  }

  for (const auto i : Squares)
  {
    params.emplace_back("bishop_pst_mg", bishop_pst_mg[i], 0, step);
    params.emplace_back("bishop_pst_eg", bishop_pst_eg[i], 0, step);
  }

  for (const auto i : Squares)
  {
    params.emplace_back("rook_pst_mg", rook_pst_mg[i], 0, step);
    params.emplace_back("rook_pst_eg", rook_pst_eg[i], 0, step);
  }

  for (const auto i : Squares)
  {
    params.emplace_back("queen_pst_mg", queen_pst_mg[i], 0, step);
    params.emplace_back("queen_pst_eg", queen_pst_eg[i], 0, step);
  }
  for (const auto i : Squares)
  {
    params.emplace_back("king_pst_mg", king_pst_mg[i], 0, step);
    params.emplace_back("king_pst_eg", king_pst_eg[i], 0, step);
  }
  /*
                  //----------------------------------
                  for (auto i = 0; i < 2; ++i)
                          params.emplace_back(Param("pawn_isolated_mg", eval_->pawn_isolated_mg[i], 0, step));

                  for (auto i = 0; i < 2; ++i)
                          params.emplace_back(Param("pawn_isolated_eg", eval_->pawn_isolated_eg[i], 0, step));

                  for (auto i = 0; i < 2; ++i)
                          params.emplace_back(Param("pawn_behind_mg", eval_->pawn_behind_mg[i], 0, step));

                  for (auto i = 0; i < 2; ++i)
                          params.emplace_back(Param("pawn_behind_eg", eval_->pawn_behind_eg[i], 0, step));

                  for (auto i = 0; i < 2; ++i)
                          params.emplace_back(Param("pawn_doubled_mg", eval_->pawn_doubled_mg[i], 0, step));

                  for (auto i = 0; i < 2; ++i)
                          params.emplace_back(Param("pawn_doubled_eg", eval_->pawn_doubled_eg[i], 0, step));

                  //----------------------------------
                  for (auto i = 0; i < 8; ++i)
                          params.emplace_back(Param("passed_pawn_mg", eval_->passed_pawn_mg[i], 0, step));

                  for (auto i = 0; i < 8; ++i)
                          params.emplace_back(Param("passed_pawn_eg", eval_->passed_pawn_eg[i], 0, step));

                  for (auto i = 0; i < 8; ++i)
                          params.emplace_back(Param("passed_pawn_no_us", eval_->passed_pawn_no_us[i], 0, step));

                  for (auto i = 0; i < 8; ++i)
                          params.emplace_back(Param("passed_pawn_no_them", eval_->passed_pawn_no_them[i], 0, step));

                  for (auto i = 0; i < 8; ++i)
                          params.emplace_back(Param("passed_pawn_no_attacks", eval_->passed_pawn_no_attacks[i], 0, step));

                  for (auto i = 0; i < 8; ++i)
                          params.emplace_back(Param("passed_pawn_king_dist_us", eval_->passed_pawn_king_dist_us[i], 0, step));

                  for (auto i = 0; i < 8; ++i)
                          params.emplace_back(Param("passed_pawn_king_dist_them", eval_->passed_pawn_king_dist_them[i], 0, step));

                  //----------------------------------
                  for (auto i = 0; i < 4; ++i)
                          params.emplace_back(Param("king_pawn_shelter", eval_->king_pawn_shelter[i], 0, step));

                  for (auto i = 0; i < 4; ++i)
                          params.emplace_back(Param("king_on_open", eval_->king_on_open[i], 0, step));

                  for (auto i = 0; i < 4; ++i)
                          params.emplace_back(Param("king_on_half_open", eval_->king_on_half_open[i], 0, step));

                  //----------------------------------
                  params.emplace_back(Param("bishop_pair_mg", eval_->bishop_pair_mg, 0, step));
                  params.emplace_back(Param("bishop_pair_eg", eval_->bishop_pair_eg, 0, step));

                  //----------------------------------
                  params.emplace_back(Param("king_obstructs_rook", eval_->king_obstructs_rook, 0, step));

                  params.emplace_back(Param("rook_open_file", eval_->rook_open_file, 0, step));

                  //----------------------------------
                  params.emplace_back(Param("knight_in_danger", eval_->knight_in_danger, 0, step));
                  params.emplace_back(Param("bishop_in_danger", eval_->bishop_in_danger, 0, step));
                  params.emplace_back(Param("rook_in_danger", eval_->rook_in_danger, 0, step));
                  params.emplace_back(Param("queen_in_danger", eval_->queen_in_danger, 0, step));

                  //----------------------------------
                  params.emplace_back(Param("knight_attack_king", eval_->knight_attack_king, 0, step));
                  params.emplace_back(Param("bishop_attack_king", eval_->bishop_attack_king, 0, step));
                  params.emplace_back(Param("rook_attack_king", eval_->rook_attack_king, 0, step));
                  params.emplace_back(Param("queen_attack_king", eval_->queen_attack_king, 0, step));
  */
}

double Tune::e(const std::vector<Node> &nodes, const std::vector<Param> &params, const std::vector<ParamIndexRecord> &params_index, double K) {
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
  const auto score = score_static_
                       ? Eval::tune(game_, nullptr, -100000, 100000)
                       : get_quiesce_score(-32768, 32768, false, 0);

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

void Tune::unmake_move() const { game_->unmake_move(); }

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

  for (auto i  = ply + 1; i < pv_length[ply]; ++i)
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
    else if (game_->board.see_move(m) >= 0) // TODO : create See on the fly?
      move_data.score = 160000 + value_captured * 20 - value_piece;
    else
      move_data.score = -100000 + value_captured * 20 - value_piece;
  } else
    exit(0);
}

}
