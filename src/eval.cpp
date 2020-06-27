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

#include <memory>
#include <bit>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include "eval.h"
#include "types.h"
#include "game.h"
#include "parameters.h"
#include "bitboard.h"
#include "magic.h"
#include "position.h"
#include "pawnhashtable.h"
#include "score.h"

namespace {

constexpr Score ZERO = Score(0);

constexpr Bitboard CenterBB = make_bitboard(d4, e4, d5, e5);

constexpr auto get_actual_eval = [](const std::array<int, COL_NB> &e) { return e[WHITE] - e[BLACK]; };

constexpr auto max_log_file_size = 1048576 * 5;
constexpr auto max_log_files     = 3;

std::shared_ptr<spdlog::logger> eval_logger = spdlog::rotating_logger_mt("eval_logger", "logs/eval.txt", max_log_file_size, max_log_files);

}// namespace

template<bool Tuning>
struct Evaluate {

  Evaluate()                      = delete;
  ~Evaluate()                     = default;
  Evaluate(const Evaluate &other) = delete;
  Evaluate(Evaluate &&other)      = delete;
  Evaluate &operator=(const Evaluate &) = delete;
  Evaluate &operator=(Evaluate &&other) = delete;
  Evaluate(const Game *g, PawnHashTable *pawntable) : b(g->board), pos(g->pos), pawnt(pawntable) {}

  template<Color Us>
  int evaluate(int alpha, int beta);

private:
  template<PieceType Pt, Color Us>
  void set_attacks(Bitboard attacks);
  template<Color C, typename... PieceTypes>
  [[nodiscard]]
  Bitboard attacked_by(PieceTypes... piece_types) const noexcept;
  void eval_pawns_both_sides();
  template<Color Us>
  void eval_material();
  template<PieceType Pt, Color Us>
  void eval_pieces();
  template<Color Us>
  void eval_pawns();
  template<Color Us>
  void eval_king();
  template<Color Us>
  void eval_passed_pawns();
  template<Color Us>
  void eval_king_attack();
  template<Color Us>
  void init_evaluate();

  const Board &b{};
  Position *pos{};
  PawnHashTable *pawnt{};
  PawnHashEntry *pawnp{};

  std::array<int, COL_NB> poseval_mg{};
  std::array<int, COL_NB> poseval_eg{};
  std::array<int, COL_NB> poseval{};
  std::array<int, COL_NB> pawn_eval_mg{};
  std::array<int, COL_NB> pawn_eval_eg{};
  std::array<int, COL_NB> passed_pawn_files{};
  std::array<int, COL_NB> attack_counter{};
  std::array<int, COL_NB> attack_count{};
  Bitboard piece_attacks[COL_NB][PieceType_Nb]{};
  std::array<Bitboard, COL_NB> king_area{};
  std::array<Bitboard, COL_NB> half_open_files{};
  Bitboard open_files{};
};

template<bool Tuning>
template<Color Us>
int Evaluate<Tuning>::evaluate(const int alpha, const int beta) {

  init_evaluate<WHITE>();
  init_evaluate<BLACK>();

  eval_material<WHITE>();
  eval_material<BLACK>();

  const auto mat_eval = get_actual_eval(poseval);

#if !defined(NO_EVAL_LAZY_THRESHOLD)

  if (const auto lazy_eval = pos->side_to_move == WHITE ? mat_eval : -mat_eval; lazy_eval - lazy_margin > beta || lazy_eval + lazy_margin < alpha)
    return pos->material.evaluate(pos->flags, lazy_eval, pos->side_to_move, &b);

#endif

  // Pass 1.
  eval_pawns_both_sides();
  eval_pieces<Knight, WHITE>();
  eval_pieces<Knight, BLACK>();
  eval_pieces<Bishop, WHITE>();
  eval_pieces<Bishop, BLACK>();
  eval_pieces<  Rook, WHITE>();
  eval_pieces<  Rook, BLACK>();
  eval_pieces< Queen, WHITE>();
  eval_pieces< Queen, BLACK>();

  eval_king<WHITE>();
  eval_king<BLACK>();

  // Pass 2.
  eval_passed_pawns<WHITE>();
  eval_passed_pawns<BLACK>();
  eval_king_attack<WHITE>();
  eval_king_attack<BLACK>();

  const auto stage = (pos->material.value() - pos->material.pawn_value()) / static_cast<double>(Material::max_value_without_pawns);

  poseval[pos->side_to_move] += tempo;

  const auto pos_eval_mg = static_cast<int>(get_actual_eval(poseval_mg) * stage);
  const auto pos_eval_eg = static_cast<int>(get_actual_eval(poseval_eg) * (1 - stage));
  const auto pos_eval    = pos_eval_mg + pos_eval_eg + get_actual_eval(poseval);
  const auto eval        = pos->material.evaluate(pos->flags, pos->side_to_move == BLACK ? -pos_eval : pos_eval, pos->side_to_move, &b);

  return eval;
}

template<bool Tuning>
template<PieceType Pt, Color Us>
void Evaluate<Tuning>::set_attacks(const Bitboard attacks) {
  constexpr auto Them = ~Us;
  piece_attacks[Us][AllPieceTypes] |= attacks;
  piece_attacks[Us][Pt] |= attacks;
  if constexpr (Pt != King)
  {
    if (const auto attacks_king = attacks & king_area[Them]; attacks_king)
    {
      attack_counter[Us] += std::popcount(attacks_king) * attacks_on_king[Pt];
      ++attack_count[Us];
    }
  }
}

template<bool Tuning>
template<Color C, typename... PieceTypes>
Bitboard Evaluate<Tuning>::attacked_by(PieceTypes... piece_types) const noexcept {
  return (... | piece_attacks[C][piece_types]);
}

template<bool Tuning>
void Evaluate<Tuning>::eval_pawns_both_sides() {

  if (pos->material.pawn_count() == 0)
  {
    pawnp = nullptr;
    return;
  }

  pawnp = pawnt->find(pos);

  if (pawnp->zkey == 0 || pawnp->zkey != pos->pawn_structure_key)
  {
    pawn_eval_mg.fill(0);
    pawn_eval_eg.fill(0);
    passed_pawn_files.fill(0);

    eval_pawns<WHITE>();
    eval_pawns<BLACK>();

    if constexpr (!Tuning)
      pawnp = pawnt->insert(pos->pawn_structure_key, get_actual_eval(pawn_eval_mg), get_actual_eval(pawn_eval_eg), passed_pawn_files);
  }
  poseval_mg[0] += pawnp->eval_mg;
  poseval_eg[0] += pawnp->eval_eg;
}

template<bool Tuning>
template<Color Us>
void Evaluate<Tuning>::eval_material() {
  poseval[Us] = pos->material.material_value[Us];

  bool add{};

  if (const auto bishop_count = pos->material.count(Us, Bishop); bishop_count == 2)
  {
    auto bishops = b.pieces(Bishop, Us);
    add          = is_opposite_colors(pop_lsb(&bishops), pop_lsb(&bishops));
  } else if (bishop_count > 2)// edge case with more than two bishops
  {
    auto bishops = b.pieces(Bishop, Us);
    std::array<bool, COL_NB> cols{};
    while (bishops)
    {
      const auto sq      = pop_lsb(&bishops);
      cols[color_of(sq)] = true;
    }

    add = cols[WHITE] & cols[BLACK];
  }

  if (add)
  {
    poseval_mg[Us] += bishop_pair.mg();
    poseval_eg[Us] += bishop_pair.eg();
  }
}

template<bool Tuning>
template<PieceType Pt, Color Us>
void Evaluate<Tuning>::eval_pieces() {
  static_assert(Pt != Pawn && Pt != King && Pt != NoPieceType);
  constexpr auto Them   = ~Us;
  const auto all_pieces = b.pieces();
  auto result           = ZeroScor;
  auto score_pos            = 0;
  auto pieces           = b.pieces(Pt, Us);
  auto attacks          = ZeroBB;

  while (pieces)
  {
    const auto sq     = pop_lsb(&pieces);
    const auto flipsq = relative_square(Them, sq);

    if constexpr (Pt == Knight || Pt == King)
      attacks = piece_attacks_bb<Pt>(sq);
    else if constexpr (Pt == Bishop)
      attacks = piece_attacks_bb<Pt>(sq, all_pieces ^ b.pieces(Queen, Them));
    else if constexpr (Pt == Rook)
      attacks = piece_attacks_bb<Pt>(sq, all_pieces ^ b.pieces(Queen, Them) ^ b.pieces(Rook, Them));
    else if constexpr (Pt == Queen)
      attacks = piece_attacks_bb<Pt>(sq, all_pieces);

    set_attacks<Pt, Us>(attacks);

    const auto free_squares          = attacks & ~b.pieces(Us);
    const auto mob                   = std::popcount(free_squares);
    const auto not_defended_by_pawns = std::popcount(free_squares & ~attacked_by<Them>(Pawn));

    if constexpr (Pt == Knight)
    {
      result += knight_pst[flipsq];
      result += knight_mob[mob];
      result += knight_mob2[not_defended_by_pawns];

      if (attacked_by<Them>(Pawn) & sq)
        score_pos -= piece_in_danger[Pt];

    } else if constexpr (Pt == Bishop)
    {
      result += bishop_pst[flipsq];
      result += bishop_mob[mob];
      result += bishop_mob2[not_defended_by_pawns];

      if (more_than_one(piece_attacks_bb<Bishop>(sq, b.pieces(Pawn, Us) | b.pieces(Pawn, Them)) & CenterBB))
        result += bishop_diagonal;

      if (attacked_by<Them>(Pawn) & sq)
        score_pos -= piece_in_danger[Pt];

    } else if constexpr (Pt == Rook)
    {
      result += rook_pst[flipsq];
      result += rook_mob[mob];

      if (open_files & sq)
        score_pos += rook_open_file;

      if (attacked_by<Them>(Pawn, Knight, Bishop) & sq)
        score_pos -= piece_in_danger[Pt];

    } else if constexpr (Pt == Queen)
    {
      result += queen_pst[flipsq];
      result += queen_mob[mob];

      if (attacked_by<Them>(Pawn, Knight, Bishop, Rook) & sq)
        score_pos -= piece_in_danger[Pt];
    }
  }

  poseval[Us] += score_pos;
  poseval_mg[Us] += result.mg();
  poseval_eg[Us] += result.eg();
}

template<bool Tuning>
template<Color Us>
void Evaluate<Tuning>::eval_pawns() {
  constexpr auto Them = ~Us;
  auto result         = ZeroScor;

  auto pawns = b.pieces(Pawn, Us);

  while (pawns)
  {
    const auto sq     = pop_lsb(&pawns);
    const auto file   = file_of(sq);
    const auto flipsq = relative_square(Them, sq);

    if (b.is_pawn_passed(sq, Us))
      passed_pawn_files[Us] |= 1 << file;

    const auto open_file = !b.is_piece_on_file(Pawn, sq, Them);

    if (b.is_pawn_isolated(sq, Us))
      result += pawn_isolated[open_file];
    else if (b.is_pawn_behind(sq, Us))
      result += pawn_behind[open_file];

    if (pawns & file)
      result += pawn_doubled[open_file];

    result += pawn_pst[flipsq];
  }
  pawn_eval_mg[Us] += result.mg();
  pawn_eval_eg[Us] += result.eg();
}

template<bool Tuning>
template<Color Us>
void Evaluate<Tuning>::eval_king() {
  constexpr Direction Up = Us == WHITE ? NORTH : SOUTH;
  const auto sq          = b.king_sq(Us);
  const auto bbsq        = bit(sq);
  const auto flipsq      = relative_square(~Us, sq);
  auto result         = king_pst[flipsq];
  auto pos_score   = 0;

  pos_score += king_pawn_shelter[std::popcount((pawn_push<Up>(bbsq) | pawn_west_attacks[Us](bbsq) | pawn_east_attacks[Us](bbsq)) & b.pieces(Pawn, Us))];

  const auto eastwest = bbsq | west_one(bbsq) | east_one(bbsq);

  pos_score += king_on_open[std::popcount(open_files & eastwest)];
  pos_score += king_on_half_open[std::popcount(half_open_files[Us] & eastwest)];

  if (((Us == 0) && (((sq == f1 || sq == g1) && (b.pieces(Rook, WHITE) & h1)) || ((sq == c1 || sq == b1) && (b.pieces(Rook, WHITE) & a1))))
      || ((Us == 1) && (((sq == f8 || sq == g8) && (b.pieces(Rook, BLACK) & h8)) || ((sq == c8 || sq == b8) && (b.pieces(Rook, BLACK) & a8)))))
    pos_score -= king_obstructs_rook;

  result += make_score(pos_score, 0);

  poseval_mg[Us] += result.mg();
  poseval_eg[Us] += result.eg();
}

template<bool Tuning>
template<Color Us>
void Evaluate<Tuning>::eval_passed_pawns() {
  constexpr auto Them = ~Us;

  if (pawnp == nullptr)
    return;

  auto result = ZeroScor;
  auto score_pos = 0;

  const auto our_pawns = b.pieces(Pawn, Us);

  for (auto files = pawnp->passed_pawn_file(Us); files; reset_lsb(files))
  {
    for (auto bb = bb_file(lsb(files)) & our_pawns; bb; reset_lsb(bb))
    {
      const auto sq         = lsb(bb);
      const auto front_span = pawn_front_span[Us][sq];
      const auto r          = relative_rank(Us, sq);
      result += passed_pawn[r];

      score_pos += passed_pawn_no_us[r] * (front_span & b.pieces(Us) ? 0 : 1);
      score_pos += passed_pawn_no_them[r] * (front_span & b.pieces(Them) ? 0 : 1);
      score_pos += passed_pawn_no_attacks[r] * (front_span & attacked_by<Them>(AllPieceTypes) ? 0 : 1);
      score_pos += passed_pawn_king_dist_them[distance(sq, b.king_sq(Them))];
      score_pos += passed_pawn_king_dist_us[distance(sq, b.king_sq(Us))];
    }
  }

  result += make_score(0, score_pos);
  poseval_mg[Us] += result.mg();
  poseval_eg[Us] += result.eg();
}

template<bool Tuning>
template<Color Us>
void Evaluate<Tuning>::eval_king_attack() {
  if (attack_count[Us] > 1)
    poseval_mg[Us] += attack_counter[Us] * (attack_count[Us] - 1);
}

template<bool Tuning>
template<Color Us>
void Evaluate<Tuning>::init_evaluate() {

  constexpr auto Them    = ~Us;
  pos->flags             = 0;
  poseval[Us]            = 0;
  poseval_mg[Us]         = 0;
  poseval_eg[Us]         = 0;
  attack_count[Us]       = 0;
  attack_counter[Us]     = 0;
  const auto ksq         = b.king_sq(Us);
  const auto attacks     = king_attacks[ksq];
  const auto our_pawns   = b.pieces(Pawn, Us);
  const auto their_pawns = b.pieces(Pawn, Them);
  open_files             = ~(pawn_fill[Us](pawn_fill[Them](our_pawns)) | pawn_fill[Us](pawn_fill[Them](their_pawns)));
  half_open_files[Us]    = ~north_fill(south_fill(our_pawns)) & ~open_files;

  set_attacks<Pawn, Us>(pawn_east_attacks[Us](our_pawns) | pawn_west_attacks[Us](our_pawns));
  set_attacks<King, Us>(attacks);
  king_area[Us] = attacks | ksq;
}

namespace Eval {

int evaluate(Game *g, PawnHashTable *pawnTable, const int alpha, const int beta) {
  return g->pos->side_to_move == WHITE ? Evaluate<false>(g, pawnTable).evaluate<WHITE>(alpha, beta) : Evaluate<false>(g, pawnTable).evaluate<BLACK>(alpha, beta);
}

int tune(Game *g, PawnHashTable *pawnTable, const int alpha, const int beta) {
  return g->pos->side_to_move == WHITE ? Evaluate<true>(g, pawnTable).evaluate<WHITE>(alpha, beta) : Evaluate<true>(g, pawnTable).evaluate<BLACK>(alpha, beta);
}

}// namespace Eval