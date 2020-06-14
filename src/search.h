#pragma once

#include <algorithm>
#include <cassert>
#include <atomic>
#include <optional>
#include "moves.h"
#include "uci.h"
#include "game.h"
#include "eval.h"
#include "see.h"
#include "hash.h"
#include "stopwatch.h"

class Search final : public MoveSorter {
public:
  Search(Protocol *p, Game *g, Eval *e, See *s) : lag_buffer(-1), verbosity(true), protocol(p), game(g), eval(e), board(g->pos->board), see(s) { }

  Search(Game *g, Eval *e, See *s) : Search(nullptr, g, e, s) {
    stop_search.store(false);
  }

  virtual ~Search() = default;

  int go(const int wtime, const int btime, const int movestogo, const int winc, const int binc, const int movetime, const int num_workers) {
    num_workers_ = num_workers;
    init_search(wtime, btime, movestogo, winc, binc, movetime);

    drawScore_[pos->side_to_move]  = 0;//-25;
    drawScore_[!pos->side_to_move] = 0;// 25;

    auto alpha = -MAXSCORE;
    auto beta  = MAXSCORE;

    while (!stop_search && search_depth < MAXDEPTH)
    {
      try
      {
        search_depth++;

        do
        {
          pv_length[0] = 0;

          get_hash_and_evaluate(alpha, beta);

          const auto score = search<EXACT, true>(search_depth, alpha, beta);

          if (score > alpha && score < beta)
            break;

          check_time();

          alpha = std::max<int>(-MAXSCORE, score - 100);
          beta  = std::min<int>(MAXSCORE, score + 100);
        } while (true);

        store_pv();

        if (move_is_easy())
          break;

        alpha = std::max<int>(-MAXSCORE, pv[0][0].score - 20);
        beta  = std::min<int>(MAXSCORE, pv[0][0].score + 20);
      } catch (const int)
      {
        while (plies)
          unmake_move();

        if (pv_length[0])
          store_pv();
      }
    }
    return 0;
  }

  void stop() { stop_search.store(true); }

  virtual void run() { go(0, 0, 0, 0, 0, 0, 0); }

protected:
  template<NodeType NT, bool PV>
  int search(const int depth, int alpha, const int beta) {
    if constexpr (!PV)
    {
      if (is_hash_score_valid(depth, alpha, beta))
        return search_node_score(pos->transp_score);
    }

    if (plies >= MAXDEPTH - 1)
      return search_node_score(pos->eval_score);

    if constexpr (!PV)
    {
      if (should_try_null_move(depth, beta))
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

    auto best_move  = 0;
    auto best_score = -MAXSCORE;
    auto move_count = 0;

    while (auto *const move_data = pos->next_move())
    {
      int score;

      if (make_move_and_evaluate(move_data->move, alpha, beta))
      {
        ++move_count;

        if (protocol && plies == 1 && search_depth >= 20 && (search_time > 5000 || is_analysing()))
          protocol->post_curr_move(move_data->move, move_count);

        if (PV && move_count == 1)
          score = search_next_depth<EXACT, true>(next_depth_pv(singular_move, depth, move_data), -beta, -alpha);
        else
        {
          const auto next_depth = next_depth_not_pv<NT, PV>(depth, move_count, move_data, alpha, best_score);

          if (!next_depth.has_value())
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
  int search_next_depth(const int depth, const int alpha, const int beta) {
    if ((pos->is_draw() || game->is_repetition()) && !is_null_move(pos->last_move))
      return search_node_score(-draw_score());
    return depth <= 0 ? -search_quiesce<PV>(alpha, beta, 0) : -search<NT, PV>(depth, alpha, beta);
  }

  template<bool PV>
  uint32_t get_singular_move(const int depth) {
    if constexpr (!PV)
      return 0;
    else
    {
      if (pos->transp_move && pos->transp_type == EXACT && depth >= 4 && search_fail_low(depth / 2, std::max<int>(-MAXSCORE, pos->eval_score - 75), pos->transp_move))
        return pos->transp_move;
      return 0;
    }
  }

  bool search_fail_low(const int depth, const int alpha, const uint32_t exclude_move) {
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

        if (!next_depth.has_value())
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
    if (move_count == 0)
      return false;
    return true;
  }

  [[nodiscard]] bool should_try_null_move([[maybe_unused]] const int depth, const int beta) const {
    return !pos->in_check && pos->null_moves_in_row < 1 && !pos->material.is_kx(pos->side_to_move) && pos->eval_score >= beta;
  }

  template<NodeType NT, bool PV>
  std::optional<int> next_depth_not_pv(const int depth, const int move_count, const MoveData *move_data, const int alpha, int &best_score) const {

    constexpr auto move_count_limit = PV ? 5 : 3;

    const auto m = move_data->move;

    if (pos->in_check && see->see_last_move(m) >= 0)
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

  int next_depth_pv(const uint32_t singular_move, const int depth, const MoveData *move_data) const {
    const auto m = move_data->move;

    if (m == singular_move)
      return depth;

    if ((pos->in_check || is_passed_pawn_move(m)) && see->see_last_move(m) >= 0)
      return depth;
    return depth - 1;
  }

  [[nodiscard]]
  static constexpr int null_move_reduction(const int depth) { return 4 + depth / 4; }

  template<bool PV>
  int search_quiesce(int alpha, const int beta, const int qs_ply) {
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

        if (pos->eval_score + piece_value(moveCaptured(move_data->move)) + 150 < alpha)
        {
          best_score = std::max<int>(best_score, pos->eval_score + piece_value(moveCaptured(move_data->move)) + 150);
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

        //const auto score = pos->is_draw()
        //                       ? -draw_score()
        //                       : -search_quiesce(-beta, -alpha, qs_ply + 1, search_pv && move_count == 1);

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

  bool make_move_and_evaluate(const uint32_t m, const int alpha, const int beta) {
    if (game->make_move(m, true, true))
    {
      pos = game->pos;
      ++plies;
      pv_length[plies] = plies;
      ++node_count;

      check_sometimes();

      get_hash_and_evaluate(-beta, -alpha);

      max_ply = std::max<int>(max_ply, plies);
      return true;
    }
    return false;
  }

  void unmake_move() {
    game->unmake_move();
    pos = game->pos;
    plies--;
  }

  void check_sometimes() {
    if ((node_count & 0x3fff) == 0)
      check_time();
  }

  void check_time() {
    if (protocol)
    {
      if (!is_analysing() && !protocol->is_fixed_depth())
        stop_search = search_depth > 1 && start_time.elapsed_milliseconds() > static_cast<unsigned>(search_time);
      else
        protocol->check_input();
    }

    if (stop_search.load())
      throw 1;
  }

  [[nodiscard]] int is_analysing() const { return protocol ? protocol->is_analysing() : true; }

  template<NodeType NT>
  void update_pv(const uint32_t move, const int score, const int depth) {
    auto *const entry = &pv[plies][plies];

    entry->score     = score;
    entry->depth     = depth;
    entry->key       = pos->key;
    entry->move      = move;
    entry->node_type = NT;
    entry->eval      = pos->eval_score;

    pv_length[plies] = pv_length[plies + 1];

    for (auto i = plies + 1; i < pv_length[plies]; ++i)
      pv[plies][i] = pv[plies + 1][i];

    if (plies == 0)
    {
      pos->pv_length = pv_length[0];

      if (protocol && verbosity)
      {
        char buf[2048], buf2[16];
        buf[0] = 0;

        for (auto i = plies; i < pv_length[plies]; ++i)
          _snprintf(&buf[strlen(buf)], sizeof buf - strlen(buf), "%s ", game->move_to_string(pv[plies][i].move, buf2));

        protocol->post_pv(search_depth, max_ply, node_count * num_workers_, nodes_per_second(), std::max<int>(1ull, start_time.elapsed_milliseconds()), TT.get_load(), score, buf, NT);
      }
    }
  }

  void store_pv() {
    assert(pv_length[0] > 0);

    for (auto i = 0; i < pv_length[0]; ++i)
    {
      const auto &entry = pv[0][i];
      TT.insert(entry.key, entry.depth, entry.score, entry.node_type, entry.move, entry.eval);
    }
  }

  [[nodiscard]] uint64_t nodes_per_second() const {
    const uint64_t micros = start_time.elapsed_microseconds();
    return micros == 0 ? node_count * num_workers_ : node_count * num_workers_ * 1000000 / micros;
  }

  void update_history_scores(const uint32_t move, const int depth) {
    history_scores[move_piece(move)][move_to(move)] += depth * depth;

    if (history_scores[move_piece(move)][move_to(move)] > 2048)
    {
      for (auto &history_score : history_scores)
        for (auto &k : history_score)
          k >>= 2;
    }
  }

  void update_killer_moves(const uint32_t move) {
    // Same move can be stored twice for a ply.
    if (!is_capture(move) && !is_promotion(move) && move != killer_moves[0][plies])
    {
      killer_moves[2][plies] = killer_moves[1][plies];
      killer_moves[1][plies] = killer_moves[0][plies];
      killer_moves[0][plies] = move;
    }
  }

  [[nodiscard]]
  bool is_killer_move(const uint32_t m, const int ply) const { return m == killer_moves[0][ply] || m == killer_moves[1][ply] || m == killer_moves[2][ply]; }

  [[nodiscard]]
  int codec_t_table_score(const int score, const int ply) const {
    if (std::abs(static_cast<int>(score)) < MAXSCORE - 128)
      return score;
    return score < 0 ? score - ply : score + ply;
  }

  void init_search(int wtime, int btime, const int movestogo, int winc, int binc, const int movetime) {
    pos                     = game->pos;// Updated in makeMove and unmakeMove from here on.
    const auto time_reserve = 72;

    if (protocol)
    {
      if (protocol->is_fixed_time())
        search_time = 950 * movetime / 1000;
      else
      {
        auto moves_left = movestogo > 30 ? 30 : movestogo;

        if (moves_left == 0)
          moves_left = 30;

        time_left = pos->side_to_move == 0 ? wtime : btime;
        time_inc  = pos->side_to_move == 0 ? winc : binc;

        if (time_inc == 0 && time_left < 1000)
        {
          search_time = time_left / (moves_left * 2);
          n_          = 1;
        } else
        {
          search_time = 2 * (time_left / (moves_left + 1) + time_inc);
          n_          = 2.5;
        }
        search_time = std::max<int>(0, std::min<int>(search_time, time_left - time_reserve));
      }
      TT.init_search();
      stop_search.store(false);
      start_time.start();
    }
    plies          = 0;
    search_depth   = 0;
    node_count     = 1;
    max_ply        = 0;
    pos->pv_length = 0;
    std::memset(pv, 0, sizeof pv);
    std::memset(killer_moves, 0, sizeof killer_moves);
    std::memset(history_scores, 0, sizeof history_scores);
    std::memset(counter_moves, 0, sizeof counter_moves);
  }

  void sort_move(MoveData &move_data) override {
    const auto m = move_data.move;

    if (pos->transp_move == m)
      move_data.score = 890010;
    else if (is_queen_promotion(m))
      move_data.score = 890000;
    else if (is_capture(m))
    {
      const auto value_captured = piece_value(moveCaptured(m));
      auto value_piece    = piece_value(move_piece(m));

      if (value_piece == 0)
        value_piece = 1800;

      if (value_piece <= value_captured)
        move_data.score = 300000 + value_captured * 20 - value_piece;
      else if (see->see_move(m) >= 0)
        move_data.score = 160000 + value_captured * 20 - value_piece;
      else
        move_data.score = -100000 + value_captured * 20 - value_piece;
    } else if (is_promotion(m))
      move_data.score = PROMOTIONMOVESCORE + piece_value(move_promoted(m));
    else if (m == killer_moves[0][plies])
      move_data.score = KILLERMOVESCORE + 20;
    else if (m == killer_moves[1][plies])
      move_data.score = KILLERMOVESCORE + 19;
    else if (m == killer_moves[2][plies])
      move_data.score = KILLERMOVESCORE + 18;
    else if (pos->last_move && counter_moves[move_piece(pos->last_move)][move_to(pos->last_move)] == m)
      move_data.score = 60000;
    else
      move_data.score = history_scores[move_piece(m)][move_to(m)];
  }

  constexpr static int search_node_score(const int score) { return score; }

  [[nodiscard]]
  int store_search_node_score(const int score, const int depth, const NodeType node_type, const uint32_t move) const {
    store_hash(depth, score, node_type, move);
    return search_node_score(score);
  }

  [[nodiscard]]
  int draw_score() const { return drawScore_[pos->side_to_move]; }

  void store_hash(const int depth, int score, const NodeType node_type, const uint32_t move) const {
    score = codec_t_table_score(score, plies);

    if (node_type == BETA)
      pos->eval_score = std::max<int>(pos->eval_score, score);
    else if (node_type == ALPHA)
      pos->eval_score = std::min<int>(pos->eval_score, score);
    else if (node_type == EXACT)
      pos->eval_score = score;

    pos->transposition = TT.insert(pos->key, depth, score, node_type, move, pos->eval_score);
  }

  void get_hash_and_evaluate(const int alpha, const int beta) const {
    if ((pos->transposition = TT.find(pos->key)) == nullptr)
    {
      pos->eval_score  = eval->evaluate(alpha, beta);
      pos->transp_type = Void;
      pos->transp_move = 0;
      return;
    }
    pos->transp_score = codec_t_table_score(pos->transposition->score, -plies);
    pos->eval_score   = codec_t_table_score(pos->transposition->eval, -plies);
    pos->transp_depth = pos->transposition->depth;
    pos->transp_type  = static_cast<NodeType>(pos->transposition->flags & 7);
    pos->transp_move  = pos->transposition->move;
    pos->flags        = 0;
  }

  [[nodiscard]]
  bool is_hash_score_valid(const int depth, const int alpha, const int beta) const {
    return pos->transposition && pos->transp_depth >= depth
           && (pos->transp_type & EXACT || (pos->transp_type & BETA && pos->transp_score >= beta) || (pos->transp_type & ALPHA && pos->transp_score <= alpha));
  }

  static constexpr NodeType node_type(const int score, const int beta, const uint32_t move) { return move ? (score >= beta ? BETA : EXACT) : ALPHA; }

  [[nodiscard]]
  bool move_is_easy() const {
    if (protocol)
    {
      if ((pos->move_count() == 1 && search_depth > 9) || (protocol->is_fixed_depth() && protocol->get_depth() == search_depth) || pv[0][0].score == MAXSCORE - 1)
        return true;

      if (!is_analysing() && !protocol->is_fixed_depth() && search_time < start_time.elapsed_milliseconds() * n_)
        return true;
    }
    return false;
  }

  [[nodiscard]]
  bool is_passed_pawn_move(const uint32_t m) const { return move_piece_type(m) == Pawn && board->is_pawn_passed(move_to(m), move_side(m)); }

public:
  struct PVEntry {
    uint64_t key;
    int depth;
    int score;
    uint32_t move;
    NodeType node_type;
    int eval;
  };

  double n_{};
  int plies{};
  int max_ply{};
  PVEntry pv[128][128]{};
  int pv_length[128]{};
  Stopwatch start_time{};
  int search_time{};
  int time_left{};
  int time_inc{};
  int lag_buffer;
  std::atomic_bool stop_search;
  bool verbosity{};
  Protocol *protocol;

protected:
  int search_depth{};
  uint32_t killer_moves[4][128]{};
  int history_scores[16][64]{};
  uint32_t counter_moves[16][64]{};
  int drawScore_[2]{};
  Game *game;
  Eval *eval;
  Board *board;
  See *see;
  Position *pos{};

  uint64_t node_count{};
  uint64_t num_workers_{};

  static constexpr int MAXSCORE = 0x7fff;
  static constexpr int MAXDEPTH = 96;

  static constexpr int KILLERMOVESCORE    = 124900;
  static constexpr int PROMOTIONMOVESCORE = 50000;

  static constexpr std::array<int, 4> futility_margin {150, 150, 150, 400};
  static constexpr std::array<int, 4> razor_margin {0, 125, 125, 400};
};
