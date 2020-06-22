#include <cassert>

#include <assert.h>
#include "search.h"
#include "eval.h"

namespace {

void store_pv(const PVEntry *pv, const int pv_length) {
  assert(pv_length > 0);

  for (auto i = 0; i < pv_length; ++i)
  {
    const auto &entry = pv[i];
    TT.insert(entry.key, entry.depth, entry.score, entry.node_type, entry.move, entry.eval);
  }
}

[[nodiscard]]
int codec_t_table_score(const int score, const int ply) {
  return std::abs(score) < Search::MAXSCORE - MAXDEPTH
       ? score
       : score < 0 ? score - ply : score + ply;
}

void get_hash_and_evaluate(Position *pos, Game* game, PawnHashTable *pawn_hash_table, const int alpha, const int beta, const int plies) {
  if ((pos->transposition = TT.find(pos->key)) == nullptr)
  {
    pos->eval_score  = Eval::evaluate(game, pawn_hash_table, alpha, beta);
    pos->transp_type = Void;
    pos->transp_move = MOVE_NONE;
    return;
  }

  pos->transp_score = codec_t_table_score(pos->transposition->score, -plies);
  pos->eval_score   = codec_t_table_score(pos->transposition->eval, -plies);
  pos->transp_depth = pos->transposition->depth;
  pos->transp_type  = static_cast<NodeType>(pos->transposition->flags & 7);
  pos->transp_move  = pos->transposition->m();
  pos->flags        = 0;
}

}

int Search::go(const SearchLimits &limits, const std::size_t num_workers) {
  num_workers_ = num_workers;
  init_search(limits);

  drawScore_[pos->side_to_move]  = 0;//-25;
  drawScore_[~pos->side_to_move] = 0;// 25;

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

        get_hash_and_evaluate(pos, game, pawn_hash_, alpha, beta, plies);

        const auto score = search<EXACT, true>(search_depth, alpha, beta);

        if (score > alpha && score < beta)
          break;

        check_time();

        alpha = std::max<int>(-MAXSCORE, score - 100);
        beta  = std::min<int>(MAXSCORE, score + 100);
      }
      while (true);

      store_pv(pv[0], pv_length.front());

      if (move_is_easy())
        break;

      alpha = std::max<int>(-MAXSCORE, pv[0][0].score - 20);
      beta  = std::min<int>(MAXSCORE, pv[0][0].score + 20);
    } catch (const int)
    {
      while (plies)
        unmake_move();

      if (const int pv_len = pv_length.front(); pv_len)
        store_pv(pv[0], pv_len);
    }
  }
  return 0;
}

void Search::stop() { stop_search.store(true); }

void Search::run() {
  const SearchLimits limits{};
  go(limits, 0);
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

  if ((pos->in_check || board->is_passed_pawn_move(m)) && board->see_last_move(m) >= 0)
    return depth;
  return depth - 1;
}

bool Search::make_move_and_evaluate(const Move m, const int alpha, const int beta) {
  if (!game->make_move(m, true, true))
    return false;

  pos = game->pos;
  ++plies;
  pv_length[plies] = plies;
  ++node_count;

  check_sometimes();

  get_hash_and_evaluate(pos, game, pawn_hash_, -beta, -alpha, plies);

  max_ply = std::max<int>(max_ply, plies);
  return true;
}

void Search::unmake_move() {
  game->unmake_move();
  pos = game->pos;
  plies--;
}

void Search::check_sometimes() {
  if ((node_count & 0x3fff) == 0)
    check_time();
}

void Search::check_time() {
  if (protocol)
  {
    if (!is_analysing() && !protocol.value()->is_fixed_depth())
      stop_search = search_depth > 1 && start_time.elapsed_milliseconds() > search_time;
    else
      protocol.value()->check_input();
  }

  if (stop_search.load())
    throw 1;
}

int Search::is_analysing() const { return protocol ? protocol.value()->is_analysing() : true; }

uint64_t Search::nodes_per_second() const {
  const auto micros = start_time.elapsed_microseconds();
  return micros == 0 ? node_count * num_workers_ : node_count * num_workers_ * 1000000 / micros;
}

void Search::update_history_scores(const Move move, const int depth) {
  history_scores[move_piece(move)][move_to(move)] += depth * depth;

  if (history_scores[move_piece(move)][move_to(move)] > 2048)
  {
    for (auto &history_score : history_scores)
      for (auto &k : history_score)
        k >>= 2;
  }
}

void Search::update_killer_moves(const Move move) {
  // Same move can be stored twice for a ply.
  if (!is_capture(move) && !is_promotion(move) && move != killer_moves[0][plies])
  {
    killer_moves[2][plies] = killer_moves[1][plies];
    killer_moves[1][plies] = killer_moves[0][plies];
    killer_moves[0][plies] = move;
  }
}

bool Search::is_killer_move(const Move m, const int ply) const { return m == killer_moves[0][ply] || m == killer_moves[1][ply] || m == killer_moves[2][ply]; }

void Search::init_search(const SearchLimits &limits) {
  pos                     = game->pos;// Updated in makeMove and unmakeMove from here on.
  const auto time_reserve = 72;

  if (protocol)
  {
    if (protocol.value()->is_fixed_time())
      search_time = 950 * limits.movetime / 1000;
    else
    {
      auto moves_left = limits.movestogo > 30 ? 30 : limits.movestogo;

      if (moves_left == 0)
        moves_left = 30;

      time_left = limits.time[pos->side_to_move];
      time_inc  = limits.inc[pos->side_to_move];

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
  pv_length.fill(0);
  std::memset(pv, 0, sizeof pv);
  std::memset(killer_moves, 0, sizeof killer_moves);
  std::memset(history_scores, 0, sizeof history_scores);
  std::memset(counter_moves, 0, sizeof counter_moves);
}

void Search::sort_move(MoveData &move_data) {
  const auto m = move_data.move;

  if (pos->transp_move == m)
    move_data.score = 890010;
  else if (is_queen_promotion(m))
    move_data.score = 890000;
  else if (is_capture(m))
  {
    const auto value_captured = piece_value(move_captured(m));
    auto value_piece          = piece_value(move_piece(m));

    if (value_piece == 0)
      value_piece = 1800;

    if (value_piece <= value_captured)
      move_data.score = 300000 + value_captured * 20 - value_piece;
    else if (board->see_move(m) >= 0)
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

int Search::store_search_node_score(const int score, const int depth, const NodeType node_type, const Move move) const {
  store_hash(depth, score, node_type, move);
  return search_node_score(score);
}

int Search::draw_score() const { return drawScore_[pos->side_to_move]; }

void Search::store_hash(const int depth, int score, const NodeType node_type, const Move move) const {
  score = codec_t_table_score(score, plies);

  if (node_type == BETA)
    pos->eval_score = std::max<int>(pos->eval_score, score);
  else if (node_type == ALPHA)
    pos->eval_score = std::min<int>(pos->eval_score, score);
  else if (node_type == EXACT)
    pos->eval_score = score;

  pos->transposition = TT.insert(pos->key, depth, score, node_type, move, pos->eval_score);
}

bool Search::is_hash_score_valid(const HashEntry *tt, const int depth, const int alpha, const int beta) const {
  return tt && tt->depth >= depth
         && (tt->is_exact() || (tt->is_beta() && pos->transp_score >= beta) || (tt->is_alpha() && pos->transp_score <= alpha));
}

bool Search::move_is_easy() const {
  if (protocol)
  {
    if ((pos->move_count() == 1 && search_depth > 9) || (protocol.value()->is_fixed_depth() && protocol.value()->get_depth() == search_depth) || pv[0][0].score == MAXSCORE - 1)
      return true;

    if (!is_analysing() && !protocol.value()->is_fixed_depth() && search_time < start_time.elapsed_milliseconds() * n_)
      return true;
  }
  return false;
}
