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
#include "board.h"
#include "parameters.h"
#include "bitboard.h"
#include "magic.h"
#include "position.h"
#include "pawnhashtable.h"
#include "score.h"
#include "datapool.h"

namespace {

constexpr auto get_actual_eval   = [](const std::array<int, COL_NB> &e) { return e[WHITE] - e[BLACK]; };
constexpr Bitboard CenterBB      = make_bitboard(d4, e4, d5, e5);
constexpr auto max_log_file_size = 1048576 * 5;
constexpr auto max_log_files     = 3;

std::shared_ptr<spdlog::logger> eval_logger = spdlog::rotating_logger_mt("eval_logger", "logs/eval.txt", max_log_file_size, max_log_files);

[[nodiscard]]
auto get_stages(Material &mat) {
  const auto stage = (mat.value() - mat.pawn_value()) / static_cast<double>(Material::max_value_without_pawns);
  return std::make_pair(stage, 1 - stage);
}

}// namespace

template<bool Tuning>
struct Evaluate {

  Evaluate()                            = delete;
  ~Evaluate()                           = default;
  Evaluate(const Evaluate &other)       = delete;
  Evaluate(Evaluate &&other)            = delete;
  Evaluate &operator=(const Evaluate &) = delete;
  Evaluate &operator=(Evaluate &&other) = delete;
  Evaluate(Board *board, const std::size_t pool_index) : b(board), pool_index_(pool_index) {}

  template<Color Us>
  int evaluate(int alpha, int beta);

private:
  template<PieceType Pt, Color Us>
  void set_attacks(Bitboard attacks);
  template<Color C, typename... PieceTypes>
  [[nodiscard]]
  Bitboard attacked_by(PieceTypes... piece_types) const noexcept;
  [[nodiscard]]
  Score eval_pawns_both_sides();
  template<Color Us>
  void eval_material();
  template<PieceType Pt, Color Us>
  [[nodiscard]]
  Score eval_pieces();
  template<Color Us>
  [[nodiscard]]
  Score eval_pawns();
  template<Color Us>
  [[nodiscard]]
  Score eval_king();
  template<Color Us>
  [[nodiscard]]
  Score eval_passed_pawns() const;
  template<Color Us>
  void eval_king_attack();
  template<Color Us>
  void init_evaluate();

  Board *b{};
  std::size_t pool_index_;
  PawnHashEntry *pawnp{};

  std::array<Score, COL_NB> poseval{};
  std::array<int, COL_NB> posistion_value{};
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

#if !defined(NO_EVAL_LAZY_THRESHOLD)

  const auto mat_eval = get_actual_eval(posistion_value);

  if (const auto lazy_eval = b->side_to_move() == WHITE ? mat_eval : -mat_eval; lazy_eval - lazy_margin > beta || lazy_eval + lazy_margin < alpha)
    return b->material().evaluate(b->flags(), lazy_eval, b->side_to_move(), b);

#endif

  // Pass 1.
  auto result = eval_pawns_both_sides();
  result     += eval_pieces<Knight, WHITE>() - eval_pieces<Knight, BLACK>();
  result     += eval_pieces<Bishop, WHITE>() - eval_pieces<Bishop, BLACK>();
  result     += eval_pieces<  Rook, WHITE>() - eval_pieces<  Rook, BLACK>();
  result     += eval_pieces< Queen, WHITE>() - eval_pieces< Queen, BLACK>();
  result     += eval_king<          WHITE>() - eval_king<          BLACK>();

  // Pass 2.
  result += eval_passed_pawns<WHITE>() - eval_passed_pawns<BLACK>();

  eval_king_attack<WHITE>();
  eval_king_attack<BLACK>();

  // finally add the remaining poseval scores
  result += poseval[WHITE] - poseval[BLACK];

  posistion_value[b->side_to_move()] += tempo;

  const auto [stage_mg, stage_eg] = get_stages(b->material());
  const auto pos_eval_mg          = static_cast<int>(result.mg() * stage_mg);
  const auto pos_eval_eg          = static_cast<int>(result.eg() * stage_eg);
  const auto pos_eval             = pos_eval_mg + pos_eval_eg + get_actual_eval(posistion_value);
  const auto eval                 = b->material().evaluate(b->flags(), b->side_to_move() == WHITE ? pos_eval : -pos_eval, b->side_to_move(), b);

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
Score Evaluate<Tuning>::eval_pawns_both_sides() {

  auto result = ZeroScore;

  if (b->material().pawn_count() == 0)
  {
    pawnp = nullptr;
    return result;
  }

  auto *pawnt         = &Pool[pool_index_]->pawn_hash;
  const auto pawn_key = b->pawn_key();
  pawnp               = pawnt->find(pawn_key);

  if (pawnp->zkey == 0 || pawnp->zkey != pawn_key)
  {
    passed_pawn_files.fill(0);

    const auto w_result = eval_pawns<WHITE>();
    const auto b_result = eval_pawns<BLACK>();
    result   = w_result - b_result;

    if constexpr (!Tuning)
      pawnp = pawnt->insert(pawn_key, result, passed_pawn_files);
  } else
    result = pawnp->eval;

  return result;
}

template<bool Tuning>
template<Color Us>
void Evaluate<Tuning>::eval_material() {

  posistion_value[Us] = b->material().material_value[Us];
  auto add            = false;

  if (const auto bishop_count = b->material().count(Us, Bishop); bishop_count == 2)
  {
    auto bishops = b->pieces(Bishop, Us);
    add          = is_opposite_colors(pop_lsb(&bishops), pop_lsb(&bishops));
  } else if (bishop_count > 2)// edge case with more than two bishops
  {
    auto bishops = b->pieces(Bishop, Us);
    std::array<bool, COL_NB> cols{};
    while (bishops)
    {
      const auto sq      = pop_lsb(&bishops);
      cols[color_of(sq)] = true;
    }

    add = cols[WHITE] & cols[BLACK];
  }

  if (add)
    poseval[Us] += bishop_pair;
}

template<bool Tuning>
template<PieceType Pt, Color Us>
Score Evaluate<Tuning>::eval_pieces() {
  static_assert(Pt != Pawn && Pt != King && Pt != NoPieceType);

  constexpr auto Them   = ~Us;
  const auto all_pieces = b->pieces();
  auto result           = ZeroScore;
  auto score_pos        = 0;
  auto pieces           = b->pieces(Pt, Us);
  auto attacks          = ZeroBB;

  while (pieces)
  {
    const auto sq     = pop_lsb(&pieces);
    const auto flipsq = relative_square(Them, sq);

    if constexpr (Pt == Knight)
      attacks = piece_attacks_bb<Pt>(sq);
    else if constexpr (Pt == Bishop)
      attacks = piece_attacks_bb<Pt>(sq, all_pieces ^ b->pieces(Queen));
    else if constexpr (Pt == Rook)
      attacks = piece_attacks_bb<Pt>(sq, all_pieces ^ b->pieces(Queen) ^ b->pieces(Rook, Us));
    else if constexpr (Pt == Queen)
      attacks = piece_attacks_bb<Pt>(sq, all_pieces);

    set_attacks<Pt, Us>(attacks);

    const auto free_squares          = attacks & ~b->pieces(Us);
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

      if (more_than_one(piece_attacks_bb<Bishop>(sq, b->pieces(Pawn)) & CenterBB))
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

      if (mob <= 3)
      {
        const auto king_file = file_of(b->king_sq(Us));
        if ((king_file < FILE_E) == (file_of(sq) < king_file))
        {
          const auto modifier = 1 + (Us & !b->castle_rights());
          result -= king_obstructs_rook * modifier;
        }
      }
    } else if constexpr (Pt == Queen)
    {
      result += queen_pst[flipsq];
      result += queen_mob[mob];

      if (attacked_by<Them>(Pawn, Knight, Bishop, Rook) & sq)
        score_pos -= piece_in_danger[Pt];
    }
  }

  posistion_value[Us] += score_pos;

  return result;
}

template<bool Tuning>
template<Color Us>
Score Evaluate<Tuning>::eval_pawns() {
  constexpr auto Them = ~Us;
  auto result         = ZeroScore;
  auto pawns          = b->pieces(Pawn, Us);

  while (pawns)
  {
    const auto sq     = pop_lsb(&pawns);
    const auto file   = file_of(sq);
    const auto flipsq = relative_square(Them, sq);

    result += pawn_pst[flipsq];

    if (b->is_pawn_passed(sq, Us))
      passed_pawn_files[Us] |= 1 << file;

    const auto open_file = !b->is_piece_on_file(Pawn, sq, Them);

    if (b->is_pawn_isolated(sq, Us))
      result += pawn_isolated[open_file];
    else if (b->is_pawn_behind(sq, Us))
      result += pawn_behind[open_file];

    if (pawns & file)
      result += pawn_doubled[open_file];
  }
  return result;
}

template<bool Tuning>
template<Color Us>
Score Evaluate<Tuning>::eval_king() {
  constexpr auto Up        = Us == WHITE ? NORTH : SOUTH;
  constexpr auto NorthEast = Us == WHITE ? NORTH_EAST : SOUTH_WEST;
  constexpr auto NorthWest = Us == WHITE ? NORTH_WEST : SOUTH_EAST;
  const auto sq            = b->king_sq(Us);
  const auto bbsq          = bit(sq);
  const auto flipsq        = relative_square(~Us, sq);
  auto result              = king_pst[flipsq];

  result += king_pawn_shelter[std::popcount((shift_bb<Up>(bbsq) | shift_bb<NorthEast>(bbsq) | shift_bb<NorthWest>(bbsq)) & b->pieces(Pawn, Us))];

  const auto east_west = bbsq | shift_bb<WEST>(bbsq) | shift_bb<EAST>(bbsq);

  result += king_on_open[std::popcount(open_files & east_west)];
  result += king_on_half_open[std::popcount(half_open_files[Us] & east_west)];

  return result;
}

template<bool Tuning>
template<Color Us>
Score Evaluate<Tuning>::eval_passed_pawns() const {
  constexpr auto Them = ~Us;
  auto result         = ZeroScore;

  if (pawnp == nullptr)
    return result;

  const auto our_pawns = b->pieces(Pawn, Us);

  for (auto files = pawnp->passed_pawn_file(Us); files; reset_lsb(files))
  {
    for (auto bb = bb_file(lsb(files)) & our_pawns; bb; reset_lsb(bb))
    {
      const auto sq         = lsb(bb);
      const auto front_span = pawn_front_span[Us][sq];
      const auto r          = relative_rank(Us, sq);
      result += passed_pawn[r];
      result += passed_pawn_no_us[r] * !(front_span & b->pieces(Us));
      result += passed_pawn_no_them[r] * !(front_span & b->pieces(Them));
      result += passed_pawn_no_attacks[r] * !(front_span & attacked_by<Them>(AllPieceTypes));
      result += passed_pawn_king_dist_them[distance(sq, b->king_sq(Them))];
      result += passed_pawn_king_dist_us[distance(sq, b->king_sq(Us))];
    }
  }

  return result;
}

template<bool Tuning>
template<Color Us>
void Evaluate<Tuning>::eval_king_attack() {
  if (attack_count[Us] > 1)
    poseval[Us] += attack_counter[Us] * (attack_count[Us] - 1);
}

template<bool Tuning>
template<Color Us>
void Evaluate<Tuning>::init_evaluate() {

  constexpr auto Them      = ~Us;
  constexpr auto NorthEast = Us == WHITE ? NORTH_EAST : SOUTH_WEST;
  constexpr auto NorthWest = Us == WHITE ? NORTH_WEST : SOUTH_EAST;
  b->flags()               = 0;
  posistion_value[Us]      = 0;
  attack_count[Us]         = 0;
  attack_counter[Us]       = 0;
  const auto ksq           = b->king_sq(Us);
  const auto attacks       = king_attacks[ksq];
  const auto our_pawns     = b->pieces(Pawn, Us);
  const auto their_pawns   = b->pieces(Pawn, Them);
  open_files               = ~(pawn_fill[Us](pawn_fill[Them](our_pawns)) | pawn_fill[Us](pawn_fill[Them](their_pawns)));
  half_open_files[Us]      = ~north_fill(south_fill(our_pawns)) & ~open_files;

  poseval.fill(ZeroScore);

  set_attacks<Pawn, Us>(shift_bb<NorthEast>(our_pawns) | shift_bb<NorthWest>(our_pawns));
  set_attacks<King, Us>(attacks);
  king_area[Us] = attacks | ksq;
}

namespace Eval {

int evaluate(Board *b, const std::size_t pool_index, const int alpha, const int beta) {
  return b->side_to_move() == WHITE
                            ? Evaluate<false>(b, pool_index).evaluate<WHITE>(alpha, beta)
                            : Evaluate<false>(b, pool_index).evaluate<BLACK>(alpha, beta);
}

int tune(Board *b, const std::size_t pool_index, const int alpha, const int beta) {
  return b->side_to_move() == WHITE
                            ? Evaluate<true>(b, pool_index).evaluate<WHITE>(alpha, beta)
                            : Evaluate<true>(b, pool_index).evaluate<BLACK>(alpha, beta);
}

}// namespace Eval