#pragma once

#include <algorithm>
#include <cassert>
#include <atomic>
#include <optional>
#include "moves.h"
#include "uci.h"
#include "game.h"
#include "stopwatch.h"
#include "position.h"
#include "pawnhashtable.h"
#include "transpositional.h"

struct PVEntry {
  uint64_t key;
  int depth;
  int score;
  uint32_t move;
  NodeType node_type;
  int eval;
};

class Search final : public MoveSorter {
public:
  Search() = delete;
  Search(std::optional<Protocol *> p, Game *g, PawnHashTable *pawnt) : lag_buffer(-1), verbosity(true), protocol(p), game(g), board(g->pos->b), pawn_hash_(pawnt) { }
  Search(Game *g, PawnHashTable *pawnt) : Search(std::nullopt, g, pawnt) {
    stop_search.store(false);
  }

  int go(const SearchLimits &limits, std::size_t num_workers);

  void stop();

  void run();

private:
  template<NodeType NT, bool PV>
  int search(int depth, int alpha, int beta);

  template<NodeType NT, bool PV>
  int search_next_depth(int depth, int alpha, int beta);

  template<bool PV>
  uint32_t get_singular_move(int depth);

  bool search_fail_low(int depth, int alpha, uint32_t exclude_move);

  [[nodiscard]]
  bool should_try_null_move(int beta) const;

  template<NodeType NT, bool PV>
  std::optional<int> next_depth_not_pv(int depth, int move_count, const MoveData *move_data, int alpha, int &best_score) const;

  int next_depth_pv(uint32_t singular_move, int depth, const MoveData *move_data) const;

  [[nodiscard]]
  static constexpr int null_move_reduction(const int depth) { return 4 + depth / 4; }

  template<bool PV>
  int search_quiesce(int alpha, int beta, int qs_ply);

  bool make_move_and_evaluate(uint32_t m, int alpha, int beta);

  void unmake_move();

  void check_sometimes();

  void check_time();

  [[nodiscard]]
  int is_analysing() const;

  template<NodeType NT>
  void update_pv(uint32_t move, int score, int depth);

  [[nodiscard]]
  uint64_t nodes_per_second() const;

  void update_history_scores(uint32_t move, int depth);

  void update_killer_moves(uint32_t move);

  [[nodiscard]]
  bool is_killer_move(uint32_t m, int ply) const;

  void init_search(const SearchLimits &limits);

  void sort_move(MoveData &move_data) override;

  constexpr static int search_node_score(const int score) { return score; }

  [[nodiscard]]
  int store_search_node_score(int score, int depth, NodeType node_type, uint32_t move) const;

  [[nodiscard]]
  int draw_score() const;

  void store_hash(int depth, int score, NodeType node_type, uint32_t move) const;

  [[nodiscard]]
  bool is_hash_score_valid(int depth, int alpha, int beta) const;

  static constexpr NodeType node_type(const int score, const int beta, const uint32_t move) { return move ? (score >= beta ? BETA : EXACT) : ALPHA; }

  [[nodiscard]]
  bool move_is_easy() const;

public:
  double n_{};
  int plies{};
  int max_ply{};
  PVEntry pv[128][128]{};
  std::array<int, 128> pv_length{};
  Stopwatch start_time{};
  TimeUnit search_time{};
  TimeUnit time_left{};
  TimeUnit time_inc{};
  int lag_buffer;
  std::atomic_bool stop_search;
  bool verbosity{};
  std::optional<Protocol *> protocol;

  static constexpr int MAXSCORE = 0x7fff;

private:

  static constexpr int MAXDEPTH = 128;

  static constexpr int KILLERMOVESCORE    = 124900;
  static constexpr int PROMOTIONMOVESCORE = 50000;

  static constexpr std::array<int, 4> futility_margin {150, 150, 150, 400};
  static constexpr std::array<int, 4> razor_margin {0, 125, 125, 400};

  int search_depth{};
  uint32_t killer_moves[4][MAXDEPTH]{};
  int history_scores[16][64]{};
  uint32_t counter_moves[16][64]{};
  std::array<int, COL_NB> drawScore_{};
  Game *game;
  Board *board;
  Position *pos{};

  uint64_t node_count{};
  std::size_t num_workers_{};

  PawnHashTable *pawn_hash_;
};

template<NodeType NT, bool PV>
int Search::search(const int depth, int alpha, const int beta) {
  if constexpr (!PV)
  {
    if (is_hash_score_valid(depth, alpha, beta))
      return search_node_score(pos->transp_score);
  }

  if (plies >= MAXDEPTH - 1)
    return search_node_score(pos->eval_score);

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
      make_move_and_evaluate(0, alpha, beta);
      const auto score = search_next_depth<ALPHA, false>(depth - null_move_reduction(depth), -beta, -beta + 1);
      unmake_move();

      if (score >= beta)
        return search_node_score(score);
    }

    if (depth <= 3 && pos->eval_score + razor_margin[depth] < beta)
    {
      const auto score = search_quiesce<false>(beta - 1, beta, 0);

      if (score < beta)
        return search_node_score(std::max<int>(score, pos->eval_score + razor_margin[depth]));
    }
  }

  const auto singular_move = get_singular_move<PV>(depth);

  pos->generate_moves(this, pos->transp_move, STAGES);

  auto best_move  = 0u;
  auto best_score = -MAXSCORE;
  auto move_count = 0;

  while (auto *const move_data = pos->next_move())
  {
    int score;

    if (make_move_and_evaluate(move_data->move, alpha, beta))
    {
      ++move_count;

      if (protocol && plies == 1 && search_depth >= 20 && (search_time > 5000 || is_analysing()))
        protocol.value()->post_curr_move(move_data->move, move_count);

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
          score = search_next_depth<EXACT, true>(next_depth_pv(0, depth, move_data), -beta, -alpha);
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
            if (plies == 0)
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

  if (move_count == 0)
    return search_node_score(pos->in_check ? -MAXSCORE + plies : draw_score());

  if (pos->reversible_half_move_count >= 100)
    return search_node_score(draw_score());

  if (best_move && !is_capture(best_move) && !is_promotion(best_move))
  {
    update_history_scores(best_move, depth);
    update_killer_moves(best_move);

    counter_moves[move_piece(pos->last_move)][move_to(pos->last_move)] = best_move;
  }
  return store_search_node_score(best_score, depth, node_type(best_score, beta, best_move), best_move);
}

template<NodeType NT, bool PV>
int Search::search_next_depth(const int depth, const int alpha, const int beta) {
  if ((pos->is_draw() || game->is_repetition()) && !is_null_move(pos->last_move))
    return search_node_score(-draw_score());
  return depth <= 0 ? -search_quiesce<PV>(alpha, beta, 0) : -search<NT, PV>(depth, alpha, beta);
}

template<bool PV>
uint32_t Search::get_singular_move(const int depth) {
  if constexpr (!PV)
    return 0;
  else
  {
    if (pos->transp_move && pos->transp_type == EXACT && depth >= 4 && search_fail_low(depth / 2, std::max<int>(-MAXSCORE, pos->eval_score - 75), pos->transp_move))
      return pos->transp_move;
    return 0;
  }
}

template<NodeType NT, bool PV>
std::optional<int> Search::next_depth_not_pv(const int depth, const int move_count, const MoveData *move_data, const int alpha, int &best_score) const {

  constexpr auto move_count_limit = PV ? 5 : 3;

  const auto m = move_data->move;

  if (pos->in_check && board->see_last_move(m) >= 0)
    return std::optional<int>(depth);

  if (move_count >= move_count_limit && !is_queen_promotion(m) && !is_capture(m) && !is_killer_move(m, plies - 1))
  {
    auto next_depth = depth - 2 - depth / 8 - (move_count - 6) / 10;

    if constexpr (NT == BETA)
      next_depth -= 2;

    if (next_depth <= 3)
    {
      const auto score = -pos->eval_score + futility_margin[std::min<int>(3, std::max<int>(0, next_depth))];

      if (score < alpha)
      {
        best_score = std::max<int>(best_score, score);
        return std::nullopt;
      }
    }
    return std::optional<int>(next_depth);
  }
  return std::optional<int>(depth - 1);
}

template<bool PV>
int Search::search_quiesce(int alpha, const int beta, const int qs_ply) {
  if constexpr (!PV)
  {
    if (is_hash_score_valid(0, alpha, beta))
      return search_node_score(pos->transp_score);
  }

  if (pos->eval_score >= beta)
  {
    return !pos->transposition || pos->transp_depth <= 0
             ? store_search_node_score(pos->eval_score, 0, BETA, 0)
             : search_node_score(pos->eval_score);
  }

  if (plies >= MAXDEPTH - 1 || qs_ply > 6)
    return search_node_score(pos->eval_score);

  auto best_move  = 0;
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
          score = move_count == 1
                    ? -search_quiesce<true>(-beta, -alpha, qs_ply + 1)
                    : -search_quiesce<false>(-beta, -alpha, qs_ply + 1);
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

  if (!pos->transposition || pos->transp_depth <= 0)
    return store_search_node_score(best_score, 0, node_type(best_score, beta, best_move), best_move);
  return search_node_score(best_score);
}

template<NodeType NT>
void Search::update_pv(const uint32_t move, const int score, const int depth) {
  auto *const entry = &pv[plies][plies];

  entry->score     = score;
  entry->depth     = depth;
  entry->key       = pos->key;
  entry->move      = move;
  entry->node_type = NT;
  entry->eval      = pos->eval_score;

  pv_length[plies] = pv_length[plies + 1];

  for (auto i    = plies + 1; i < pv_length[plies]; ++i)
    pv[plies][i] = pv[plies + 1][i];

  if (plies == 0)
  {
    pos->pv_length = pv_length[0];

    if (protocol && verbosity)
    {
      fmt::memory_buffer buffer;

      for (auto i = plies; i < pv_length[plies]; ++i)
        fmt::format_to(buffer, "{} ", game->move_to_string(pv[plies][i].move));

      protocol.value()->post_pv(search_depth, max_ply, node_count * num_workers_, nodes_per_second(), start_time.elapsed_milliseconds() + 1, TT.get_load(), score, buffer, NT);
    }
  }
}
