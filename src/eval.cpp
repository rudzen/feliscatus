/*
  Feliscatus, a UCI chess playing engine derived from Tomcat 1.0 (Bobcat 8.0)
  Copyright (C) 2008-2016 Gunnar Harms (Bobcat author)
  Copyright (C) 2017      FireFather (Tomcat author)
  Copyright (C) 2020-2022 Rudy Alex Kohn

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

#include "eval.hpp"
#include "types.hpp"
#include "board.hpp"
#include "parameters.hpp"

namespace
{

constexpr auto max_log_file_size = 1048576 * 5;
constexpr auto max_log_files     = 3;

const std::shared_ptr<spdlog::logger> eval_logger =
  spdlog::rotating_logger_mt("eval_logger", "logs/eval.txt", max_log_file_size, max_log_files);

[[nodiscard]]
auto stages(Material &mat)
{
  const auto stage = (mat.value() - mat.pawn_value()) / static_cast<double>(Material::max_value_without_pawns);
  return std::make_pair(stage, 1 - stage);
}

constexpr bool is_bishop_squares_colors_disparate(Bitboard bishops)
{
  std::array<bool, COL_NB> cols{};
  while (bishops)
  {
    const auto sq      = pop_lsb(&bishops);
    cols[color_of(sq)] = true;
  }

  return cols[WHITE] & cols[BLACK];
}

}   // namespace

template<bool Tuning>
struct Evaluate
{

  Evaluate()                      = delete;
  ~Evaluate()                     = default;
  Evaluate(const Evaluate &other) = delete;
  Evaluate(Evaluate &&other)      = delete;
  Evaluate &operator=(const Evaluate &) = delete;
  Evaluate &operator=(Evaluate &&other) = delete;
  Evaluate(const Board *board, const std::size_t pool_index) : b(board), pool_index_(pool_index)
  { }

  template<Color Us>
  int evaluate(int alpha, int beta);

private:
  template<PieceType Pt, Color Us>
  void set_attacks(Bitboard attacks);
  template<Color C, typename... PieceTypes>
  [[nodiscard]]
  Bitboard attacked_by(PieceTypes... piece_types) const noexcept;
  template<Color Us>
  void eval_material();
  template<PieceType Pt, Color Us>
  [[nodiscard]]
  Score eval_pieces();
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
  void init();
  int actual_eval() const {
    return posistion_value[WHITE] - posistion_value[BLACK];
  };

  const Board *b{};
  PawnHashEntry *phe;
  std::size_t pool_index_;

  std::array<Score, COL_NB> poseval{};
  std::array<int, COL_NB> posistion_value{};
  std::array<int, COL_NB> attack_counter{};
  std::array<int, COL_NB> attack_count{};
  Bitboard piece_attacks[COL_NB][PIECETYPE_NB]{};
  std::array<Bitboard, COL_NB> king_area{};

};

template<bool Tuning>
template<Color Us>
int Evaluate<Tuning>::evaluate(const int alpha, const int beta)
{
  init();

  init_evaluate<WHITE>();
  init_evaluate<BLACK>();

  eval_material<WHITE>();
  eval_material<BLACK>();

#if !defined(NO_EVAL_LAZY_THRESHOLD)

  const auto mat_eval = actual_eval();

  if (const auto lazy_eval = Us == WHITE ? mat_eval : -mat_eval;
      lazy_eval - params::lazy_margin > beta || lazy_eval + params::lazy_margin < alpha)
    return b->material().evaluate<Us>(b->flags(), lazy_eval, b);

#endif

  auto result = ZeroScore;

  // Pass 1.
  result += phe->eval();
  result += eval_pieces<KNIGHT, WHITE>() - eval_pieces<KNIGHT, BLACK>();
  result += eval_pieces<BISHOP, WHITE>() - eval_pieces<BISHOP, BLACK>();
  result += eval_pieces<ROOK, WHITE>() - eval_pieces<ROOK, BLACK>();
  result += eval_pieces<QUEEN, WHITE>() - eval_pieces<QUEEN, BLACK>();
  result += eval_king<WHITE>() - eval_king<BLACK>();

  // Pass 2.
  result += eval_passed_pawns<WHITE>() - eval_passed_pawns<BLACK>();

  eval_king_attack<WHITE>();
  eval_king_attack<BLACK>();

  // finally add the remaining poseval scores
  result += poseval[WHITE] - poseval[BLACK];

  if constexpr (Us == WHITE)
    posistion_value[Us] += params::tempo;

  const auto [stage_mg, stage_eg] = stages(b->material());
  const auto pos_eval_mg          = static_cast<int>(result.mg() * stage_mg);
  const auto pos_eval_eg          = static_cast<int>(result.eg() * stage_eg);
  const auto pos_eval             = pos_eval_mg + pos_eval_eg + actual_eval();
  const auto eval                 = b->material().evaluate<Us>(b->flags(), Us == WHITE ? pos_eval : -pos_eval, b);

  return eval;
}

template<bool Tuning>
template<PieceType Pt, Color Us>
void Evaluate<Tuning>::set_attacks(const Bitboard attacks)
{
  constexpr auto Them = ~Us;
  piece_attacks[Us][ALL_PIECE_TYPES] |= attacks;
  piece_attacks[Us][Pt] |= attacks;
  if constexpr (Pt != KING)
  {
    if (const auto attacks_king = attacks & king_area[Them]; attacks_king)
    {
      attack_counter[Us] += popcount(attacks_king) * params::attacks_on_king[Pt];
      ++attack_count[Us];
    }
  }
}

template<bool Tuning>
template<Color C, typename... PieceTypes>
Bitboard Evaluate<Tuning>::attacked_by(PieceTypes... piece_types) const noexcept
{
  return (... | piece_attacks[C][piece_types]);
}

template<bool Tuning>
template<Color Us>
void Evaluate<Tuning>::eval_material()
{
  posistion_value[Us] = b->material().material_value[Us];
  auto add            = false;

  if (const auto bishop_count = b->piece_count(Us, BISHOP); bishop_count == 2)
  {
    auto bishops = b->pieces(BISHOP, Us);
    add          = is_opposite_colors(lsb(bishops), msb(bishops));
  } else if (bishop_count > 2)   // edge case with more than two bishops
  {
    const auto bishops = b->pieces(BISHOP, Us);
    add = is_bishop_squares_colors_disparate(bishops);
  }

  if (add)
    poseval[Us] += params::bishop_pair;
}

template<bool Tuning>
template<PieceType Pt, Color Us>
Score Evaluate<Tuning>::eval_pieces()
{
  static_assert(Pt != PAWN && Pt != KING && Pt != NO_PT);

  constexpr auto Them   = ~Us;
  const auto all_pieces = b->pieces();
  auto result           = ZeroScore;
  auto score_pos        = 0;
  auto pieces           = b->pieces(Pt, Us);
  auto attacks          = ZeroBB;

  while (pieces)
  {
    const auto s      = pop_lsb(&pieces);
    const auto flip_s = relative_square(Them, s);

    if constexpr (Pt == KNIGHT)
      attacks = piece_attacks_bb<Pt>(s);
    else if constexpr (Pt == BISHOP)
      attacks = piece_attacks_bb<Pt>(s, all_pieces ^ b->pieces(QUEEN));
    else if constexpr (Pt == ROOK)
      attacks = piece_attacks_bb<Pt>(s, all_pieces ^ b->pieces(QUEEN) ^ b->pieces(ROOK, Us));
    else if constexpr (Pt == QUEEN)
      attacks = piece_attacks_bb<Pt>(s, all_pieces);

    set_attacks<Pt, Us>(attacks);

    const auto free_squares          = attacks & ~b->pieces(Us);
    const auto mob                   = popcount(free_squares);
    const auto not_defended_by_pawns = popcount(free_squares & ~attacked_by<Them>(PAWN));

    result += params::pst<Pt>(flip_s);

    if constexpr (Pt == KNIGHT)
    {
      result += params::knight_mob[mob];
      result += params::knight_mob2[not_defended_by_pawns];

      if (attacked_by<Them>(PAWN) & s)
        score_pos -= params::piece_in_danger[Pt];

    } else if constexpr (Pt == BISHOP)
    {
      result += params::bishop_mob[mob];
      result += params::bishop_mob2[not_defended_by_pawns];

      if (more_than_one(piece_attacks_bb<BISHOP>(s, b->pieces(PAWN)) & CenterBB))
        result += params::bishop_diagonal;

      if (attacked_by<Them>(PAWN) & s)
        score_pos -= params::piece_in_danger[Pt];

    } else if constexpr (Pt == ROOK)
    {
      result += params::rook_mob[mob];

      if (phe->open_files[Us] & s)
        score_pos += params::rook_open_file;

      if (attacked_by<Them>(PAWN, KNIGHT, BISHOP) & s)
        score_pos -= params::piece_in_danger[Pt];

      if (mob <= 3)
      {
        const auto king_file = file_of(b->square<KING>(Us));
        if ((king_file < FILE_E) == (file_of(s) < king_file))
        {
          const auto modifier = 1 + (Us & !b->can_castle());
          result -= params::king_obstructs_rook * modifier;
        }
      }
    } else if constexpr (Pt == QUEEN)
    {
      result += params::queen_mob[mob];

      if (attacked_by<Them>(PAWN, KNIGHT, BISHOP, ROOK) & s)
        score_pos -= params::piece_in_danger[Pt];
    }
  }

  posistion_value[Us] += score_pos;

  return result;
}

template<bool Tuning>
template<Color Us>
Score Evaluate<Tuning>::eval_king()
{
  constexpr auto Up        = Us == WHITE ? NORTH : SOUTH;
  constexpr auto NorthEast = Us == WHITE ? NORTH_EAST : SOUTH_WEST;
  constexpr auto NorthWest = Us == WHITE ? NORTH_WEST : SOUTH_EAST;
  const auto ksq           = b->square<KING>(Us);
  const auto bb            = bit(ksq);
  const auto flip_ksq      = relative_square(~Us, ksq);
  auto result              = params::pst<KING>(flip_ksq);

  result += params::king_pawn_shelter[popcount((shift_bb<Up>(bb) | shift_bb<NorthEast>(bb) | shift_bb<NorthWest>(bb)) & b->pieces(PAWN, Us))];

  const auto east_west = bb | shift_bb<WEST>(bb) | shift_bb<EAST>(bb);

  result += params::king_on_open[popcount(phe->open_files[Us] & east_west)];
  result += params::king_on_half_open[popcount(phe->half_open_files[Us] & east_west)];

  return result;
}

template<bool Tuning>
template<Color Us>
Score Evaluate<Tuning>::eval_passed_pawns() const
{
  constexpr auto Them      = ~Us;
  auto result              = ZeroScore;
  auto pp                  = phe->passed_pawns[Us];
  const auto enemy_attacks = attacked_by<Them>(ALL_PIECE_TYPES);
  const auto ksq           = b->square<KING>(Us);
  const auto theirKsq      = b->square<KING>(Them);

  while (pp)
  {
    const auto s          = pop_lsb(&pp);
    const auto front_span = pawn_front_spanBB(Us, s);
    const auto r          = relative_rank(Us, s);
    result += params::passed_pawn[r];
    result += params::passed_pawn_no_us[r] * !(front_span & b->pieces(Us));
    result += params::passed_pawn_no_them[r] * !(front_span & b->pieces(Them));
    result += params::passed_pawn_no_attacks[r] * !(front_span & enemy_attacks);
    result += params::passed_pawn_king_dist_them[distance(s, theirKsq)];
    result += params::passed_pawn_king_dist_us[distance(s, ksq)];
  }

  return result;
}

template<bool Tuning>
template<Color Us>
void Evaluate<Tuning>::eval_king_attack()
{
  if (attack_count[Us] > 1)
    poseval[Us] += attack_counter[Us] * (attack_count[Us] - 1);
}

template<bool Tuning>
template<Color Us>
void Evaluate<Tuning>::init_evaluate()
{
  posistion_value[Us]      = 0;
  attack_count[Us]         = 0;
  attack_counter[Us]       = 0;
  const auto ksq           = b->square<KING>(Us);
  const auto attacks       = all_attacks<KING>(ksq);

  set_attacks<PAWN, Us>(phe->pawn_attacks[Us]);
  set_attacks<KING, Us>(attacks);

  const auto s{make_square(std::clamp(file_of(ksq), FILE_B, FILE_G), std::clamp(rank_of(ksq), RANK_2, RANK_7))};
  king_area[Us] = attacks | s;
}

template<bool Tuning>
void Evaluate<Tuning>::init()
{

  b->flags() = 0;
  poseval.fill(ZeroScore);
  phe = Pawn::at<Tuning>(b);

}

namespace Eval
{

int evaluate(Board *b, const std::size_t pool_index, const int alpha, const int beta)
{
  return b->side_to_move() == WHITE ? Evaluate<false>(b, pool_index).evaluate<WHITE>(alpha, beta)
                                    : Evaluate<false>(b, pool_index).evaluate<BLACK>(alpha, beta);
}

int tune(Board *b, const std::size_t pool_index, const int alpha, const int beta)
{
  return b->side_to_move() == WHITE ? Evaluate<true>(b, pool_index).evaluate<WHITE>(alpha, beta)
                                    : Evaluate<true>(b, pool_index).evaluate<BLACK>(alpha, beta);
}

}   // namespace Eval