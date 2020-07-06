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

#include <span>
#include <cassert>
#include <assert.h>
#include <algorithm>
#include <optional>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include "search.h"
#include "eval.h"
#include "transpositional.h"
#include "pv_entry.h"
#include "position.h"
#include "tpool.h"
#include "uci.h"
#include "board.h"

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
constexpr NodeType node_type(const int score, const int beta, const Move m) {
  return m ? (score >= beta ? BETA : EXACT) : ALPHA;
}

[[nodiscard]]
constexpr int null_move_reduction(const int d) {
  return 4 + d / 4;
}

void store_pv(const std::array<PVEntry, MAXDEPTH> &pv, const int pv_length) {
  assert(pv_length > 0);
  std::for_each(
    pv.begin(), std::next(pv.begin(), pv_length), [&](const PVEntry &entry) { TT.insert(entry.key, entry.depth, entry.score, entry.node_type, entry.move, entry.eval); });
}

[[nodiscard]]
constexpr int codec_t_table_score(const int score, const int ply) {
  return util::abs(score) < MAXSCORE - MAXDEPTH ? score : score < 0 ? score - ply : score + ply;
}

bool is_hash_score_valid(const Position *pos, const int depth, const int alpha, const int beta) {
  return pos->transposition && pos->transposition->depth() >= depth
         && (pos->transposition->is_exact() || (pos->transposition->is_beta() && pos->transp_score >= beta) || (pos->transposition->is_alpha() && pos->transp_score <= alpha));
}

void get_hash_and_evaluate(Position *pos, Board *b, const std::size_t pool_index, const int alpha, const int beta, const int plies) {
  if ((pos->transposition = TT.find(b->key())) == nullptr)
  {
    pos->eval_score  = Eval::evaluate(b, pool_index, alpha, beta);
    pos->transp_type = NO_NT;
    pos->transp_move = MOVE_NONE;
    return;
  }

  pos->transp_score = codec_t_table_score(pos->transposition->score(), -plies);
  pos->eval_score   = codec_t_table_score(pos->transposition->eval(), -plies);
  pos->transp_depth = pos->transposition->depth();
  pos->transp_type  = pos->transposition->flags();
  pos->transp_move  = pos->transposition->move();
  b->flags()        = 0;
}

[[nodiscard]]
bool is_killer_move(const Move m, const Position *p) {
  const auto &km = p->killer_moves;
  return std::find(km.cbegin(), km.cend(), m) != km.cend();
}

void update_history_scores(HistoryScores &history_scores, const Move m, const int d) {

  const auto pc = move_piece(m);
  const auto to = move_to(m);

  history_scores[pc][to] += (d * d);

  if (history_scores[pc][to] <= 2048)
    return;

  for (auto &history_score : history_scores)
    for (auto &k : history_score)
      k >>= 2;
}

void update_killer_moves(KillerMoves &km, const Move m) {
  // Same move can be stored twice for a ply.
  if (is_capture(m) || is_promotion(m) || m == km.front())
    return;

  // Rotate the killer moves, index 3 become 2, index 2 becomes 1 and index 1 becomes 0 which is replaced with new move
  // This is the same as std::copy_backward(km.begin(), std::prev(km.end(), 1), std::next(km.begin(), 1));
  std::rotate(km.begin(), std::prev(km.end(), 1), km.end());
  km[0] = m;
}

}// namespace

template<Searcher SearcherType>
struct Search final : MoveSorter {
  Search(Board *t_board) : b(t_board), t(t_board->my_thread()) { }
  ~Search()                         = default;
  Search()                          = delete;
  Search(const Search &other)       = delete;
  Search(Search &&other)            = delete;
  Search &operator=(const Search &) = delete;
  Search &operator=(Search &&other) = delete;

  int go();

private:
  template<NodeType NT, bool PV>
  int search(int depth, int alpha, int beta);

  template<NodeType NT, bool PV>
  int search_next_depth(int depth, int alpha, int beta);

  template<bool PV>
  Move get_singular_move(int depth);

  auto search_fail_low(int depth, int alpha, Move exclude);

  [[nodiscard]]
  bool should_try_null_move(int beta) const;

  template<NodeType NT, bool PV>
  [[nodiscard]]
  std::optional<int> next_depth_not_pv(int depth, int move_count, const MoveData *move_data, int alpha, int &best_score) const;

  [[nodiscard]]
  int next_depth_pv(Move singular_move, int depth, const MoveData *md) const;

  template<bool PV>
  [[nodiscard]]
  int search_quiesce(int alpha, int beta, int qs_ply);

  bool make_move_and_evaluate(Move m, int alpha, int beta);

  void unmake_move();

  void check_sometimes(uint64_t nodes) const;

  void check_time() const;

  [[nodiscard]]
  bool is_analysing() const;

  template<NodeType NT>
  void update_pv(Move m, int score, int depth) const;

  void init_search();

  void sort_move(MoveData &md) override;

  [[nodiscard]] int store_search_node_score(int score, int depth, NodeType nt, Move m) const;

  [[nodiscard]] int draw_score() const;

  void store_hash(int depth, int score, NodeType nt, Move m) const;

  [[nodiscard]] bool move_is_easy() const;

  Board *b;
  Position *pos{};
  thread *t;
  static constexpr bool verbosity = SearcherType == Searcher::Master;
};

template<Searcher SearcherType>
int Search<SearcherType>::go() {
  init_search();

  auto alpha = -MAXSCORE;
  auto beta  = MAXSCORE;

  while (!pool.stop && b->search_depth < MAXDEPTH)
  {
    try
    {
      b->search_depth++;

      do
      {
        t->pv_length[0] = 0;

        get_hash_and_evaluate(pos, b, t->index(), alpha, beta, b->plies);

        const auto score = search<EXACT, true>(b->search_depth, alpha, beta);

        if (score > alpha && score < beta)
          break;

        check_time();

        alpha = std::max<int>(-MAXSCORE, score - 100);
        beta  = std::min<int>(MAXSCORE, score + 100);
      } while (true);

      store_pv(t->pv.front(), t->pv_length.front());

      [[unlikely]] if (move_is_easy()) break;

      alpha = std::max<int>(-MAXSCORE, t->pv[0][0].score - 20);
      beta  = std::min<int>(MAXSCORE, t->pv[0][0].score + 20);
    } catch (const int)
    {
      while (b->plies)
        unmake_move();

      if (const auto pv_len = t->pv_length.front(); pv_len)
        store_pv(t->pv.front(), pv_len);
    }
  }
  return 0;
}

template<Searcher SearcherType>
template<NodeType NT, bool PV>
int Search<SearcherType>::search(int depth, int alpha, const int beta) {
  if constexpr (!PV)
  {
    if (is_hash_score_valid(pos, depth, alpha, beta))
      return pos->transp_score;
  }

  if (b->plies >= MAXDEPTH - 1)
    return pos->eval_score;

  if constexpr (!PV)
  {
    // null move reduction
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

    // Razoring
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

      if constexpr (verbosity)
      {
        if (b->plies == 1 && b->search_depth >= 20 && (pool.main()->time.should_post_curr_move() || is_analysing()))
          uci::post_curr_move(move_data->move, move_count);

        if (pool.main()->time.should_post_info())
          uci::post_info(depth, b->search_depth);
      }

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

  if (pool.stop)
    throw 1;

  if (move_count == 0)
    return b->in_check() ? -MAXSCORE + b->plies : draw_score();

  if (pos->reversible_half_move_count >= 100)
    return draw_score();

  if (best_move && !is_capture(best_move) && !is_promotion(best_move))
  {
    update_history_scores(t->history_scores, best_move, depth);
    update_killer_moves(pos->killer_moves, best_move);

    t->counter_moves[move_piece(pos->last_move)][move_to(pos->last_move)] = best_move;
  }
  return store_search_node_score(best_score, depth, node_type(best_score, beta, best_move), best_move);
}

template<Searcher SearcherType>
template<NodeType NT, bool PV>
int Search<SearcherType>::search_next_depth(const int depth, const int alpha, const int beta) {
  return (b->is_draw() || b->is_repetition()) && !is_null_move(pos->last_move) ? -draw_score()
                                                                               : depth <= 0 ? -search_quiesce<PV>(alpha, beta, 0) : -search<NT, PV>(depth, alpha, beta);
}

template<Searcher SearcherType>
template<bool PV>
Move Search<SearcherType>::get_singular_move(int depth) {
  if constexpr (!PV)
    return MOVE_NONE;
  else
  {
    return pos->transp_move
        && pos->transp_type == EXACT
        && depth >= 4 && search_fail_low(depth / 2, std::max<int>(-MAXSCORE, pos->eval_score - 75), pos->transp_move)
             ? pos->transp_move
             : MOVE_NONE;
  }
}

template<Searcher SearcherType>
auto Search<SearcherType>::search_fail_low(const int depth, int alpha, const Move exclude) {
  pos->generate_moves(this, pos->transp_move, STAGES);

  auto move_count = 0;

  while (auto *const move_data = pos->next_move())
  {
    if (pool.stop)
      return false;

    [[unlikely]] if (move_data->move == exclude) continue;

    if (make_move_and_evaluate(move_data->move, alpha, alpha + 1))
    {
      auto best_score       = -MAXSCORE;// dummy
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

  return move_count > 0;
}

template<Searcher SearcherType>
bool Search<SearcherType>::should_try_null_move(int beta) const {
  return !b->in_check()
      && pos->null_moves_in_row < 1
      && !b->material().is_kx(b->side_to_move())
      && pos->eval_score >= beta;
}

template<Searcher SearcherType>
template<NodeType NT, bool PV>
[[nodiscard]] std::optional<int> Search<SearcherType>::next_depth_not_pv(int depth, int move_count, const MoveData *move_data, int alpha, int &best_score) const {
  const auto m = move_data->move;

  if (b->in_check() && b->see_last_move(m) >= 0)
    return std::make_optional(depth);

  constexpr auto move_count_limit = PV ? 5 : 3;

  if (move_count >= move_count_limit && !is_queen_promotion(m) && !is_capture(m) && !is_killer_move(m, pos - 1))
  {
    auto next_depth = depth - 2 - depth / 8 - (move_count - 6) / 10;

    if constexpr (NT == BETA)
      next_depth -= 2;

    // futility
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

template<Searcher SearcherType>
int Search<SearcherType>::next_depth_pv(Move singular_move, int depth, const MoveData *md) const {
  const auto m = md->move;

  if (m == singular_move)
    return depth;

  if ((b->in_check() || b->is_passed_pawn_move(m)) && b->see_last_move(m) >= 0)
    return depth;
  return depth - 1;
}

template<Searcher SearcherType>
template<bool PV>
int Search<SearcherType>::search_quiesce(int alpha, int beta, int qs_ply) {
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

      if (b->is_draw())
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

  return !pos->transposition || pos->transp_depth <= 0
       ? store_search_node_score(best_score, 0, node_type(best_score, beta, best_move), best_move)
       : best_score;
}

template<Searcher SearcherType>
bool Search<SearcherType>::make_move_and_evaluate(Move m, int alpha, int beta) {
  const auto current_nodes = t->node_count.fetch_add(1, std::memory_order_relaxed);

  [[unlikely]]
  if (!b->make_move(m, true, true))
    return false;

  pos = b->pos;
  ++b->plies;
  t->pv_length[b->plies] = b->plies;

  check_sometimes(current_nodes);

  get_hash_and_evaluate(pos, b, t->index(), -beta, -alpha, b->plies);

  b->max_ply = std::max<int>(b->max_ply, b->plies);
  return true;
}

template<Searcher SearcherType>
void Search<SearcherType>::unmake_move() {
  b->unmake_move();
  pos = b->pos;
  b->plies--;
}

template<Searcher SearcherType>
void Search<SearcherType>::check_sometimes(uint64_t nodes) const {
  if constexpr (verbosity)
  {
    if (nodes >= 16383)
      check_time();
  }
}

template<Searcher SearcherType>
void Search<SearcherType>::check_time() const {
  if constexpr (verbosity)
  {
    const auto stop = !is_analysing() && !pool.main()->time.is_fixed_depth() && b->search_depth > 1 && pool.main()->time.time_up();

    if (stop)
    {
      pool.stop = true;
      throw 1;
    }
  }
}

template<Searcher SearcherType>
bool Search<SearcherType>::is_analysing() const {
  if constexpr (!verbosity)
    return true;
  else
    return pool.main()->time.is_analysing();
}

template<Searcher SearcherType>
template<NodeType NT>
void Search<SearcherType>::update_pv(Move m, int score, int depth) const {
  const auto ply      = b->plies;
  const auto next_ply = ply + 1;
  auto &pv            = t->pv;
  auto *const entry   = &pv[ply][ply];
  auto &pv_len        = t->pv_length;

  entry->score     = score;
  entry->depth     = depth;
  entry->key       = b->key();
  entry->move      = m;
  entry->node_type = NT;
  entry->eval      = pos->eval_score;

  pv_len[ply] = pv_len[next_ply];

  std::copy(std::next(pv[next_ply].begin(), next_ply), std::next(pv[next_ply].begin(), pv_len[ply]), std::next(pv[ply].begin(), next_ply));

  if (ply == 0)
  {
    pos->pv_length = pv_len[0];

    if constexpr (verbosity)
    {
      const std::span pv_line{pv[ply]};
      uci::post_pv(b->search_depth, b->max_ply, score, pv_line.subspan(ply, pv_len[ply]), NT);
    }
  }
}

template<Searcher SearcherType>
void Search<SearcherType>::init_search() {
  pos            = b->pos;// Updated in make_move and unmake_move from here on.
  pos->pv_length = 0;
  pos->killer_moves.fill(MOVE_NONE);
}

template<Searcher SearcherType>
void Search<SearcherType>::sort_move(MoveData &md) {
  const auto m = md.move;

  if (pos->transp_move == m)
    md.score = 890010;
  else if (is_queen_promotion(m))
    md.score = 890000;
  else if (is_capture(m))// also en-pessant
  {
    const auto value_captured = piece_value(move_captured(m));
    auto value_piece          = piece_value(move_piece(m));

    if (value_piece == 0)
      value_piece = 1800;

    if (value_piece <= value_captured)
      md.score = 300000 + value_captured * 20 - value_piece;
    else if (b->see_move(m) >= 0)
      md.score = 160000 + value_captured * 20 - value_piece;
    else
      md.score = -100000 + value_captured * 20 - value_piece;
  } else if (is_promotion(m))
    md.score = PROMOTIONMOVESCORE + piece_value(move_promoted(m));
  else if (m == pos->killer_moves[0])
    md.score = KILLERMOVESCORE + 20;
  else if (m == pos->killer_moves[1])
    md.score = KILLERMOVESCORE + 19;
  else if (m == pos->killer_moves[2])
    md.score = KILLERMOVESCORE + 18;
  else if (m == pos->killer_moves[3])
    md.score = KILLERMOVESCORE + 17;
  else if (pos->last_move && t->counter_moves[move_piece(pos->last_move)][move_to(pos->last_move)] == m)
    md.score = 60000;
  else
    md.score = t->history_scores[move_piece(m)][move_to(m)];
}

template<Searcher SearcherType>
int Search<SearcherType>::store_search_node_score(int score, int depth, NodeType nt, Move m) const {
  store_hash(depth, score, nt, m);
  return score;
}

template<Searcher SearcherType>
int Search<SearcherType>::draw_score() const {
  return t->draw_score[b->side_to_move()];
}

template<Searcher SearcherType>
void Search<SearcherType>::store_hash(int depth, int score, NodeType nt, Move m) const {
  score = codec_t_table_score(score, b->plies);

  if (nt == BETA)
    pos->eval_score = std::max<int>(pos->eval_score, score);
  else if (nt == ALPHA)
    pos->eval_score = std::min<int>(pos->eval_score, score);
  else if (nt == EXACT)
    pos->eval_score = score;

  pos->transposition = TT.insert(b->key(), depth, score, nt, m, pos->eval_score);
}

template<Searcher SearcherType>
bool Search<SearcherType>::move_is_easy() const {
  if constexpr (!verbosity)
    return false;
  else
  {
    if ((pos->move_count() == 1 && b->search_depth > 9) || (pool.main()->time.is_fixed_depth() && pool.main()->time.get_depth() == b->search_depth)
        || (t->pv[0][0].score == MAXSCORE - 1))
      return true;

    return !is_analysing() && !pool.main()->time.is_fixed_depth() && pool.main()->time.plenty_time();
  }
}

// basic search start

void thread::search() {
  Search<Searcher::Slave>(root_board.get()).go();
}

void main_thread::search() {

  // initialize
  time.init(root_board->side_to_move(), pool.limits);
  TT.init_search();

  pool.start_searching(); // start workers
  Search<Searcher::Master>(root_board.get()).go();

  while (!pool.stop && (ponder || pool.limits.infinite))
  { }

  pool.stop = true;

  // Wait until all threads have finished
  pool.wait_for_search_finished();

  if (root_board->pos->pv_length)
  {
    const auto [m, p_move] = std::make_pair(pv[0][0].move, pv[0][1].move ? pv[0][1].move : MOVE_NONE);
    uci::post_moves(m, p_move);
  }
}
