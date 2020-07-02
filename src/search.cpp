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
#include <assert.h>
#include <algorithm>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include "search.h"
#include "eval.h"
#include "transpositional.h"
#include "pv_entry.h"
#include "board.h"
#include "position.h"
#include "uci.h"

namespace {

constexpr auto max_log_file_size = 1048576 * 5;
constexpr auto max_log_files     = 3;

std::shared_ptr<spdlog::logger> search_logger = spdlog::rotating_logger_mt("search_logger", "logs/search.txt", max_log_file_size, max_log_files);

constexpr int KILLERMOVESCORE    = 124900;
constexpr int PROMOTIONMOVESCORE = 50000;
constexpr int MAXSCORE           = 32767;

constexpr std::array<int, 4> futility_margin{150, 150, 150, 400};
constexpr std::array<int, 4> razor_margin{0, 125, 125, 400};

[[nodiscard]]
constexpr NodeType node_type(const int score, const int beta, const Move move) {
  return move ? (score >= beta ? BETA : EXACT) : ALPHA;
}

[[nodiscard]]
constexpr int null_move_reduction(const int depth) {
  return 4 + depth / 4;
}

void store_pv(const std::array<PVEntry, MAXDEPTH> &pv, const int pv_length) {
  assert(pv_length > 0);
  std::for_each(pv.begin(), std::next(pv.begin(), pv_length), [&](const PVEntry &entry) {
    TT.insert(entry.key, entry.depth, entry.score, entry.node_type, entry.move, entry.eval);
  });
}

[[nodiscard]]
constexpr int codec_t_table_score(const int score, const int ply) {
  return util::abs(score) < MAXSCORE - MAXDEPTH
       ? score
       : score < 0 ? score - ply : score + ply;
}

bool is_hash_score_valid(const Position *pos, const int depth, const int alpha, const int beta) {
  return pos->transposition && pos->transposition->depth >= depth
      && (pos->transposition->is_exact() || (pos->transposition->is_beta() && pos->transp_score >= beta) || (pos->transposition->is_alpha() && pos->transp_score <= alpha));
}

void get_hash_and_evaluate(Position *pos, Board *b, const std::size_t pool_index, const int alpha, const int beta, const int plies) {
  if ((pos->transposition = TT.find(pos->key)) == nullptr)
  {
    pos->eval_score  = Eval::evaluate(b, pool_index, alpha, beta);
    pos->transp_type = Void;
    pos->transp_move = MOVE_NONE;
    return;
  }

  pos->transp_score = codec_t_table_score(pos->transposition->score, -plies);
  pos->eval_score   = codec_t_table_score(pos->transposition->eval, -plies);
  pos->transp_depth = pos->transposition->depth;
  pos->transp_type  = static_cast<NodeType>(pos->transposition->flags & 7);
  pos->transp_move  = pos->transposition->move;
  pos->flags        = 0;
}

bool is_killer_move(const Move m, const Position *p) {
  const auto &km = p->killer_moves;
  return std::find(km.cbegin(), km.cend(), m) != km.cend();
}

void update_history_scores(HistoryScores &history_scores, const Move move, const int depth) {

  const auto pc = move_piece(move);
  const auto to = move_to(move);

  history_scores[pc][to] += (depth * depth);

  if (history_scores[pc][to] <= 2048)
    return;

  for (auto &history_score : history_scores)
    for (auto &k : history_score)
      k >>= 2;
}

void update_killer_moves(KillerMoves &km, const Move move) {
  // Same move can be stored twice for a ply.
  if (is_capture(move) || is_promotion(move) || move == km.front())
    return;

  // Rotate the killer moves, index 3 become 2, index 2 becomes 1 and index 1 becomes 0 which is replaced with new move
  // This is the same as std::copy_backward(km.begin(), std::prev(km.end(), 1), std::next(km.begin(), 1));
  std::rotate(km.begin(), std::prev(km.end(), 1), km.end());
  km[0] = move;
}

}

int Search::go(SearchLimits &limits) {
  data_ = Pool[data_index_].get();
  init_search(limits);

  draw_score_[pos->side_to_move]  = 0;//-25;
  draw_score_[~pos->side_to_move] = 0;// 25;

  auto alpha = -MAXSCORE;
  auto beta  = MAXSCORE;

  while (!stop_search && b->search_depth < MAXDEPTH)
  {
    try
    {
      b->search_depth++;

      do
      {
        data_->pv_length[0] = 0;

        get_hash_and_evaluate(pos, b, data_index_, alpha, beta, b->plies);

        const auto score = search<EXACT, true>(b->search_depth, alpha, beta);

        if (score > alpha && score < beta)
          break;

        check_time();

        alpha = std::max<int>(-MAXSCORE, score - 100);
        beta  = std::min<int>(MAXSCORE, score + 100);
      }
      while (true);

      store_pv(data_->pv.front(), data_->pv_length.front());

      if (move_is_easy())
        break;

      alpha = std::max<int>(-MAXSCORE, data_->pv[0][0].score - 20);
      beta  = std::min<int>(MAXSCORE, data_->pv[0][0].score + 20);
    } catch (const int)
    {
      while (b->plies)
        unmake_move();

      if (const auto pv_len = data_->pv_length.front(); pv_len)
        store_pv(data_->pv.front(), pv_len);
    }
  }
  return 0;
}

void Search::stop() { stop_search.store(true); }

void Search::run() {
  SearchLimits limits{};
  go(limits);
}

bool Search::search_fail_low(const int depth, const int alpha, const Move exclude_move) {
  pos->generate_moves(this, pos->transp_move, STAGES);

  auto move_count = 0;

  while (auto *const move_data = pos->next_move())
  {
    if (move_data->move == exclude_move)
      continue;

    auto best_score = -MAXSCORE;// dummy

    if (make_move_and_evaluate(move_data->move, alpha, alpha + 1))
    {
      const auto next_depth = next_depth_not_pv<BETA, true>(depth, ++move_count, move_data, alpha, best_score);

      if (!next_depth)
      {
        unmake_move();
        continue;
      }

      auto score = search_next_depth<BETA, false>(next_depth.value(), -alpha - 1, -alpha);

      if (score > alpha && depth > 1 && next_depth.value() < depth - 1)
        score = search_next_depth<BETA, false>(depth - 1, -alpha - 1, -alpha);

      unmake_move();

      if (score > alpha)
        return false;
    }
  }

  return move_count != 0;
}

bool Search::should_try_null_move(const int beta) const {
  return !pos->in_check && pos->null_moves_in_row < 1 && !pos->material.is_kx(pos->side_to_move) && pos->eval_score >= beta;
}

int Search::next_depth_pv(const Move singular_move, const int depth, const MoveData *move_data) const {
  const auto m = move_data->move;

  if (m == singular_move)
    return depth;

  if ((pos->in_check || b->is_passed_pawn_move(m)) && b->see_last_move(m) >= 0)
    return depth;
  return depth - 1;
}

bool Search::make_move_and_evaluate(const Move m, const int alpha, const int beta) {
  if (!b->make_move(m, true, true))
    return false;

  pos = b->pos;
  ++b->plies;
  data_->pv_length[b->plies] = b->plies;
  const auto current_nodes = data_->node_count.fetch_add(1, std::memory_order_relaxed);

  check_sometimes(current_nodes);

  get_hash_and_evaluate(pos, b, data_index_, -beta, -alpha, b->plies);

  b->max_ply = std::max<int>(b->max_ply, b->plies);
  return true;
}

void Search::unmake_move() {
  b->unmake_move();
  pos = b->pos;
  b->plies--;
}

void Search::check_sometimes(const uint64_t nodes) {
  if ((nodes & 0x3fff) == 0)
    check_time();
}

void Search::check_time() {
  const auto stop = verbosity && (!is_analysing() && !Pool.time.is_fixed_depth()) && b->search_depth > 1 && Pool.time.time_up();

  if (stop)
  {
    stop_search.store(true);
    throw 1;
  }
}

int Search::is_analysing() const { return verbosity ? Pool.time.is_analysing() : true; }

void Search::init_search(SearchLimits &limits) {
  pos = b->pos;// Updated in make_move and unmake_move from here on.

  if (verbosity)
  {
    Pool.time.init(pos->side_to_move, limits);
    TT.init_search();
    stop_search.store(false);
  }
  data_->node_count  = 0;
  pos->pv_length     = 0;
  pos->killer_moves.fill(MOVE_NONE);
}

void Search::sort_move(MoveData &move_data) {
  const auto m = move_data.move;

  if (pos->transp_move == m)
    move_data.score = 890010;
  else if (is_queen_promotion(m))
    move_data.score = 890000;
  else if (is_capture(m)) // also en-pessant
  {
    const auto value_captured = piece_value(move_captured(m));
    auto value_piece          = piece_value(move_piece(m));

    if (value_piece == 0)
      value_piece = 1800;

    if (value_piece <= value_captured)
      move_data.score = 300000 + value_captured * 20 - value_piece;
    else if (b->see_move(m) >= 0)
      move_data.score = 160000 + value_captured * 20 - value_piece;
    else
      move_data.score = -100000 + value_captured * 20 - value_piece;
  } else if (is_promotion(m))
    move_data.score = PROMOTIONMOVESCORE + piece_value(move_promoted(m));
  else if (m == pos->killer_moves[0])
    move_data.score = KILLERMOVESCORE + 20;
  else if (m == pos->killer_moves[1])
    move_data.score = KILLERMOVESCORE + 19;
  else if (m == pos->killer_moves[2])
    move_data.score = KILLERMOVESCORE + 18;
  else if (m == pos->killer_moves[3])
    move_data.score = KILLERMOVESCORE + 17;
  else if (pos->last_move && data_->counter_moves[move_piece(pos->last_move)][move_to(pos->last_move)] == m)
    move_data.score = 60000;
  else
    move_data.score = data_->history_scores[move_piece(m)][move_to(m)];
}

int Search::store_search_node_score(const int score, const int depth, const NodeType node_type, const Move move) const {
  store_hash(depth, score, node_type, move);
  return score;
}

int Search::draw_score() const { return draw_score_[pos->side_to_move]; }

void Search::store_hash(const int depth, int score, const NodeType node_type, const Move move) const {
  score = codec_t_table_score(score, b->plies);

  if (node_type == BETA)
    pos->eval_score = std::max<int>(pos->eval_score, score);
  else if (node_type == ALPHA)
    pos->eval_score = std::min<int>(pos->eval_score, score);
  else if (node_type == EXACT)
    pos->eval_score = score;

  pos->transposition = TT.insert(pos->key, depth, score, node_type, move, pos->eval_score);
}

bool Search::move_is_easy() const {
  if (!verbosity)
    return false;

  if ((pos->move_count() == 1 && b->search_depth > 9)
      || (Pool.time.is_fixed_depth() && Pool.time.get_depth() == b->search_depth)
      || (data_->pv[0][0].score == MAXSCORE - 1))
    return true;

  return !is_analysing() && !Pool.time.is_fixed_depth() && Pool.time.plenty_time();
}

template<NodeType NT, bool PV>
int Search::search(const int depth, int alpha, const int beta) {
  if constexpr (!PV)
  {
    if (is_hash_score_valid(pos, depth, alpha, beta))
      return pos->transp_score;
  }

  if (b->plies >= MAXDEPTH - 1)
    return pos->eval_score;

  if constexpr (!PV)
  {
    if (should_try_null_move(beta))
    {
      if (depth <= 5)
      {
        const auto score = pos->eval_score - 50 - 100 * (depth / 2);

        if (score >= beta)
          return score;
      }
      make_move_and_evaluate(MOVE_NONE, alpha, beta);
      const auto score = search_next_depth<ALPHA, false>(depth - null_move_reduction(depth), -beta, -beta + 1);
      unmake_move();

      if (score >= beta)
        return score;
    }

    if (depth <= 3 && pos->eval_score + razor_margin[depth] < beta)
    {
      const auto score = search_quiesce<false>(beta - 1, beta, 0);

      if (score < beta)
        return std::max<int>(score, pos->eval_score + razor_margin[depth]);
    }
  }

  const auto singular_move = get_singular_move<PV>(depth);

  pos->generate_moves(this, pos->transp_move, STAGES);

  auto best_move  = MOVE_NONE;
  auto best_score = -MAXSCORE;
  auto move_count = 0;

  while (auto *const move_data = pos->next_move())
  {
    int score;

    if (make_move_and_evaluate(move_data->move, alpha, beta))
    {
      ++move_count;

      if (verbosity && b->plies == 1 && b->search_depth >= 20 && (Pool.time.should_post_curr_move() || is_analysing()))
        uci::post_curr_move(move_data->move, move_count);

      if (PV && move_count == 1)
        score = search_next_depth<EXACT, true>(next_depth_pv(singular_move, depth, move_data), -beta, -alpha);
      else
      {
        const auto next_depth = next_depth_not_pv<NT, PV>(depth, move_count, move_data, alpha, best_score);

        if (!next_depth)
        {
          unmake_move();
          continue;
        }

        constexpr auto next_expected_node_type = NT & (EXACT | ALPHA) ? BETA : ALPHA;

        score = search_next_depth<next_expected_node_type, false>(next_depth.value(), -alpha - 1, -alpha);

        if (score > alpha && depth > 1 && next_depth.value() < depth - 1)
          score = search_next_depth<next_expected_node_type, false>(depth - 1, -alpha - 1, -alpha);

        if (score > alpha && score < beta)
          score = search_next_depth<EXACT, true>(next_depth_pv(MOVE_NONE, depth, move_data), -beta, -alpha);
      }

      unmake_move();

      if (score > best_score)
      {
        best_score = score;

        if (best_score > alpha)
        {
          best_move = move_data->move;

          if (score >= beta)
          {
            if (b->plies == 0)
              update_pv<BETA>(best_move, best_score, depth);

            break;
          }
          update_pv<EXACT>(best_move, best_score, depth);
          alpha = best_score;
        }
        //          else if (pv) {
        //            updatePV(move_data->move, best_score, depth, ALPHA);
        //          }
      }
    }
  }

  if (stop_search.load(std::memory_order_relaxed))
    throw 1;

  if (move_count == 0)
    return pos->in_check ? -MAXSCORE + b->plies : draw_score();

  if (pos->reversible_half_move_count >= 100)
    return draw_score();

  if (best_move && !is_capture(best_move) && !is_promotion(best_move))
  {
    update_history_scores(data_->history_scores, best_move, depth);
    update_killer_moves(pos->killer_moves, best_move);

    data_->counter_moves[move_piece(pos->last_move)][move_to(pos->last_move)] = best_move;
  }
  return store_search_node_score(best_score, depth, node_type(best_score, beta, best_move), best_move);
}

template int Search::search<EXACT,  true>(int, int, int);
template int Search::search<EXACT, false>(int, int, int);
template int Search::search< BETA,  true>(int, int, int);
template int Search::search< BETA, false>(int, int, int);
template int Search::search<ALPHA,  true>(int, int, int);
template int Search::search<ALPHA, false>(int, int, int);

template<NodeType NT, bool PV>
int Search::search_next_depth(const int depth, const int alpha, const int beta) {
  return (pos->is_draw() || b->is_repetition()) && !is_null_move(pos->last_move) ? -draw_score()
                                                                                 : depth <= 0 ? -search_quiesce<PV>(alpha, beta, 0) : -search<NT, PV>(depth, alpha, beta);
}

template int Search::search_next_depth<EXACT,  true>(int, int, int);
template int Search::search_next_depth<EXACT, false>(int, int, int);
template int Search::search_next_depth< BETA,  true>(int, int, int);
template int Search::search_next_depth< BETA, false>(int, int, int);
template int Search::search_next_depth<ALPHA,  true>(int, int, int);
template int Search::search_next_depth<ALPHA, false>(int, int, int);

template<bool PV>
Move Search::get_singular_move(const int depth) {
  if constexpr (!PV)
    return MOVE_NONE;
  else
  {
    return pos->transp_move && pos->transp_type == EXACT && depth >= 4 && search_fail_low(depth / 2, std::max<int>(-MAXSCORE, pos->eval_score - 75), pos->transp_move)
             ? pos->transp_move
             : MOVE_NONE;
  }
}

template Move Search::get_singular_move< true>(int);
template Move Search::get_singular_move<false>(int);

template<NodeType NT, bool PV>
std::optional<int> Search::next_depth_not_pv(const int depth, const int move_count, const MoveData *move_data, const int alpha, int &best_score) const {

  const auto m = move_data->move;

  if (pos->in_check && b->see_last_move(m) >= 0)
    return std::make_optional(depth);

  constexpr auto move_count_limit = PV ? 5 : 3;

  if (move_count >= move_count_limit && !is_queen_promotion(m) && !is_capture(m) && !is_killer_move(m, pos - 1))
  {
    auto next_depth = depth - 2 - depth / 8 - (move_count - 6) / 10;

    if constexpr (NT == BETA)
      next_depth -= 2;

    if (next_depth <= 3)
    {
      const auto score = -pos->eval_score + futility_margin[std::clamp(next_depth, 0, 3)];

      if (score < alpha)
      {
        best_score = std::max<int>(best_score, score);
        return std::nullopt;
      }
    }
    return std::make_optional(next_depth);
  }
  return std::make_optional(depth - 1);
}

template std::optional<int> Search::next_depth_not_pv<EXACT,  true>(int, int, const MoveData *, int, int &) const;
template std::optional<int> Search::next_depth_not_pv<EXACT, false>(int, int, const MoveData *, int, int &) const;
template std::optional<int> Search::next_depth_not_pv< BETA,  true>(int, int, const MoveData *, int, int &) const;
template std::optional<int> Search::next_depth_not_pv< BETA, false>(int, int, const MoveData *, int, int &) const;
template std::optional<int> Search::next_depth_not_pv<ALPHA,  true>(int, int, const MoveData *, int, int &) const;
template std::optional<int> Search::next_depth_not_pv<ALPHA, false>(int, int, const MoveData *, int, int &) const;

template<bool PV>
int Search::search_quiesce(int alpha, const int beta, const int qs_ply) {
  if constexpr (!PV)
  {
    if (is_hash_score_valid(pos, 0, alpha, beta))
      return pos->transp_score;
  }

  if (pos->eval_score >= beta)
    return !pos->transposition || pos->transp_depth <= 0 ? store_search_node_score(pos->eval_score, 0, BETA, MOVE_NONE) : pos->eval_score;

  if (b->plies >= MAXDEPTH - 1 || qs_ply > 6)
    return pos->eval_score;

  auto best_move  = MOVE_NONE;
  auto best_score = pos->eval_score;
  auto move_count = 0;

  if (best_score > alpha)
    alpha = best_score;

  pos->generate_captures_and_promotions(this);

  while (auto *const move_data = pos->next_move())
  {
    if (!is_promotion(move_data->move))
    {
      if (move_data->score < 0)
        break;

      if (pos->eval_score + piece_value(move_captured(move_data->move)) + 150 < alpha)
      {
        best_score = std::max<int>(best_score, pos->eval_score + piece_value(move_captured(move_data->move)) + 150);
        continue;
      }
    }

    if (make_move_and_evaluate(move_data->move, alpha, beta))
    {
      ++move_count;

      int score;

      if (pos->is_draw())
        score = -draw_score();
      else
      {
        if constexpr (PV)
          score = move_count == 1 ? -search_quiesce<true>(-beta, -alpha, qs_ply + 1) : -search_quiesce<false>(-beta, -alpha, qs_ply + 1);
        else
          score = -search_quiesce<false>(-beta, -alpha, qs_ply + 1);
      }

      unmake_move();

      if (score > best_score)
      {
        best_score = score;

        if (best_score > alpha)
        {
          best_move = move_data->move;

          if (score >= beta)
            break;

          update_pv<EXACT>(best_move, best_score, 0);
          alpha = best_score;
        }
      }
    }
  }

  return !pos->transposition || pos->transp_depth <= 0 ? store_search_node_score(best_score, 0, node_type(best_score, beta, best_move), best_move) : best_score;
}

template int Search::search_quiesce< true>(int, int, int);
template int Search::search_quiesce<false>(int, int, int);

template<NodeType NT>
void Search::update_pv(const Move move, const int score, const int depth) const {
  auto &pv          = data_->pv;
  auto *const entry = &pv[b->plies][b->plies];

  entry->score     = score;
  entry->depth     = depth;
  entry->key       = pos->key;
  entry->move      = move;
  entry->node_type = NT;
  entry->eval      = pos->eval_score;

  const auto next_ply = b->plies + 1;

  auto &pv_len = data_->pv_length;

  pv_len[b->plies] = pv_len[next_ply];

  for (auto i = next_ply; i < pv_len[b->plies]; ++i)
    pv[b->plies][i] = pv[next_ply][i];

  if (b->plies == 0)
  {
    pos->pv_length = pv_len[0];

    if (verbosity)
      uci::post_pv(b->search_depth, b->max_ply, score, pv[b->plies], pv_len[b->plies], b->plies, NT);
  }
}

template void Search::update_pv<EXACT>(Move, int, int) const;
template void Search::update_pv< BETA>(Move, int, int) const;
template void Search::update_pv<ALPHA>(Move, int, int) const;
