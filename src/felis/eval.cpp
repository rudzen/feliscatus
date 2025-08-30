// Copyright (C) 2008-2016 Gunnar Harms (Bobcat author)
// Copyright (C) 2017      FireFather (Tomcat author)
// Copyright (C) 2020-2025 Rudy Alex Kohn
// See end of file for extended copyright information.

#include <memory>
#include <bit>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <felis/eval.hpp>
#include <felis/board.hpp>
#include <felis/parameters.hpp>

namespace {

constexpr auto max_log_file_size = 1048576 * 5;
constexpr auto max_log_files     = 3;

const std::shared_ptr<spdlog::logger> eval_logger = spdlog::rotating_logger_mt("eval_logger", "logs/eval.txt", max_log_file_size, max_log_files);

struct Stages {
  r64 mg;
  r64 eg;
};

[[nodiscard]]
Stages stages(Material* mat) {
  const r64 stage = (material::value(mat) - material::pawn_value(mat)) / static_cast<double>(material::max_value_without_pawns);
  return {.mg = stage, .eg = 1 - stage};
}

constexpr bool is_bishop_squares_colors_disparate(Bitboard bishops) {
  if (!bishops)
    return false;

  bool cols[COL_NB] = {false, false};
  while (bishops) {
    const auto sq      = pop_lsb(&bishops);
    cols[color_of(sq)] = true;
  }

  return cols[WHITE] & cols[BLACK];
}

}   // namespace

struct EvalD {
  const Board* b{};
  std::size_t pool_index_;
  PawnHashEntry* phe;
  std::array<Score, COL_NB> poseval;

  i32 posistion_value[COL_NB];
  i32 attack_counter[COL_NB];
  i32 attack_count[COL_NB];
  Bitboard piece_attacks[COL_NB][PIECETYPE_NB];
  Bitboard king_area[COL_NB];
};

template<bool Tuning>
void InitEvalD(EvalD* eval, const Board* board, const std::size_t pool_index) {
  eval->b           = board;
  eval->pool_index_ = pool_index;
  eval->phe         = Pawn::at<Tuning>(board);
  eval->poseval.fill(ZeroScore);
  std::memset(&eval->posistion_value, 0, sizeof(EvalD) - offsetof(EvalD, posistion_value));
}

template<PieceType Pt, Color Us>
void set_attacks(EvalD* eval, const Bitboard attacks) {
  constexpr auto Them = ~Us;
  eval->piece_attacks[Us][ALL_PIECE_TYPES] |= attacks;
  eval->piece_attacks[Us][Pt] |= attacks;
  if constexpr (Pt != KING) {
    if (const auto attacks_king = attacks & eval->king_area[Them]; attacks_king) {
      eval->attack_counter[Us] += popcount(attacks_king) * params::attacks_on_king[Pt];
      ++eval->attack_count[Us];
    }
  }
}

template<Color Us>
void eval_material(EvalD* eval) {
  eval->posistion_value[Us] = eval->b->material()->material_value[Us];
  bool add                  = false;

  const i32 bishop_count = eval->b->piece_count(Us, BISHOP);

  if (bishop_count == 2) {
    Bitboard bishops = eval->b->pieces(BISHOP, Us);
    add              = is_opposite_colors(lsb(bishops), msb(bishops));
  } else if (bishop_count > 2)   // edge case with more than two bishops
  {
    const Bitboard bishops = eval->b->pieces(BISHOP, Us);
    add                    = is_bishop_squares_colors_disparate(bishops);
  }

  if (add)
    eval->poseval[Us] += params::bishop_pair;
}

template<Color Us>
void init_evaluate(EvalD* eval) {
  const auto ksq     = eval->b->square<KING>(Us);
  const auto attacks = all_attacks<KING>(ksq);

  set_attacks<PAWN, Us>(eval, eval->phe->pawn_attacks[Us]);
  set_attacks<KING, Us>(eval, attacks);

  const File f   = std::clamp(file_of(ksq), FILE_B, FILE_G);
  const Rank r   = std::clamp(rank_of(ksq), RANK_2, RANK_7);
  const Square s = make_square(f, r);

  eval->king_area[Us] = attacks | s;
}

template<Color C, typename... PieceTypes>
Bitboard attacked_by(EvalD* eval, PieceTypes... piece_types) {
  return (... | eval->piece_attacks[C][piece_types]);
}

template<PieceType Pt, Color Us>
Score eval_pieces(EvalD* eval) {
  static_assert(Pt != PAWN && Pt != KING && Pt != NO_PT);

  constexpr Color Them = ~Us;

  const Board* b            = eval->b;
  const Bitboard all_pieces = b->pieces();
  Bitboard pieces           = b->pieces(Pt, Us);
  Bitboard attacks          = ZeroBB;
  i32 score_pos             = 0;
  Score result              = ZeroScore;

  while (pieces) {
    const Square s      = pop_lsb(&pieces);
    const Square flip_s = relative_square(Them, s);

    if constexpr (Pt == KNIGHT)
      attacks = piece_attacks_bb<Pt>(s);
    else if constexpr (Pt == BISHOP)
      attacks = piece_attacks_bb<Pt>(s, all_pieces ^ b->pieces(QUEEN));
    else if constexpr (Pt == ROOK)
      attacks = piece_attacks_bb<Pt>(s, all_pieces ^ b->pieces(QUEEN) ^ b->pieces(ROOK, Us));
    else if constexpr (Pt == QUEEN)
      attacks = piece_attacks_bb<Pt>(s, all_pieces);

    set_attacks<Pt, Us>(eval, attacks);

    const Bitboard free_squares     = attacks & ~b->pieces(Us);
    const i32 mob                   = popcount(free_squares);
    const i32 not_defended_by_pawns = popcount(free_squares & ~attacked_by<Them>(eval, PAWN));

    result += params::pst<Pt>(flip_s);

    if constexpr (Pt == KNIGHT) {
      result += params::knight_mob[mob];
      result += params::knight_mob2[not_defended_by_pawns];

      if (attacked_by<Them>(eval, PAWN) & s)
        score_pos -= params::piece_in_danger[Pt];

    } else if constexpr (Pt == BISHOP) {
      result += params::bishop_mob[mob];
      result += params::bishop_mob2[not_defended_by_pawns];

      if (more_than_one(piece_attacks_bb<BISHOP>(s, b->pieces(PAWN)) & CenterBB))
        result += params::bishop_diagonal;

      if (attacked_by<Them>(eval, PAWN) & s)
        score_pos -= params::piece_in_danger[Pt];

    } else if constexpr (Pt == ROOK) {
      result += params::rook_mob[mob];

      if (eval->phe->open_files[Us] & s)
        score_pos += params::rook_open_file;

      if (attacked_by<Them>(eval, PAWN, KNIGHT, BISHOP) & s)
        score_pos -= params::piece_in_danger[Pt];

      if (mob <= 3) {
        const File king_file              = file_of(b->square<KING>(Us));
        const bool kingFileLessThanFileE  = king_file < FILE_E;
        const bool squareLessThanKingFile = file_of(s) < king_file;
        if (kingFileLessThanFileE == squareLessThanKingFile) {
          const i32 modifier = 1 + (Us & !b->can_castle());
          result -= params::king_obstructs_rook * modifier;
        }
      }
    } else if constexpr (Pt == QUEEN) {
      result += params::queen_mob[mob];

      if (attacked_by<Them>(eval, PAWN, KNIGHT, BISHOP, ROOK) & s)
        score_pos -= params::piece_in_danger[Pt];
    }
  }

  eval->posistion_value[Us] += score_pos;

  return result;
}

template<Color Us>
Score eval_king(EvalD* eval) {
  constexpr auto Up        = Us == WHITE ? NORTH : SOUTH;
  constexpr auto NorthEast = Us == WHITE ? NORTH_EAST : SOUTH_WEST;
  constexpr auto NorthWest = Us == WHITE ? NORTH_WEST : SOUTH_EAST;

  const Board* b           = eval->b;
  const PawnHashEntry* phe = eval->phe;
  const Square ksq         = b->square<KING>(Us);
  const Bitboard bb        = bit(ksq);
  const Square flip_ksq    = relative_square(~Us, ksq);

  Score result = params::pst<KING>(flip_ksq);

  const Bitboard ourPawns    = b->pieces(PAWN, Us);
  const Bitboard pawnShelter = (shift_bb<Up>(bb) | shift_bb<NorthEast>(bb) | shift_bb<NorthWest>(bb)) & ourPawns;
  const i32 pawnShelterCount = popcount(pawnShelter);

  result += params::king_pawn_shelter[pawnShelterCount];

  const Bitboard east_west = bb | shift_bb<WEST>(bb) | shift_bb<EAST>(bb);

  result += params::king_on_open[popcount(phe->open_files[Us] & east_west)];
  result += params::king_on_half_open[popcount(phe->half_open_files[Us] & east_west)];

  return result;
}

template<Color Us>
Score eval_passed_pawns(EvalD* eval) {
  constexpr Color Them = ~Us;

  const Board* b           = eval->b;
  const PawnHashEntry* phe = eval->phe;

  Bitboard pp                  = phe->passed_pawns[Us];
  const Bitboard enemy_attacks = attacked_by<Them>(eval, ALL_PIECE_TYPES);
  const Square ksq             = b->square<KING>(Us);
  const Square theirKsq        = b->square<KING>(Them);
  const Bitboard ourPieces     = b->pieces(Us);
  const Bitboard theirPieces   = b->pieces(Them);

  Score result = ZeroScore;

  while (pp) {
    const Square s            = pop_lsb(&pp);
    const Bitboard front_span = pawn_front_spanBB(Us, s);
    const Rank r              = relative_rank(Us, s);

    result += params::passed_pawn[r];
    result += params::passed_pawn_no_us[r] * !(front_span & ourPieces);
    result += params::passed_pawn_no_them[r] * !(front_span & theirPieces);
    result += params::passed_pawn_no_attacks[r] * !(front_span & enemy_attacks);
    result += params::passed_pawn_king_dist_them[distance(s, theirKsq)];
    result += params::passed_pawn_king_dist_us[distance(s, ksq)];
  }

  return result;
}

template<Color Us>
void eval_king_attack(EvalD* eval) {
  if (eval->attack_count[Us] > 1)
    eval->poseval[Us] += eval->attack_counter[Us] * (eval->attack_count[Us] - 1);
}

template<Color Us>
i32 evaluate(EvalD* eval, const int alpha, const int beta) {
  init_evaluate<WHITE>(eval);
  init_evaluate<BLACK>(eval);

  eval_material<WHITE>(eval);
  eval_material<BLACK>(eval);

  const Board* b           = eval->b;
  const PawnHashEntry* phe = eval->phe;

#if !defined(NO_EVAL_LAZY_THRESHOLD)

  const auto mat_eval = eval->posistion_value[WHITE] - eval->posistion_value[BLACK];

  if (const auto lazy_eval = Us == WHITE ? mat_eval : -mat_eval; lazy_eval - params::lazy_margin > beta || lazy_eval + params::lazy_margin < alpha)
    return material::evaluate(b->material(), b->flags(), lazy_eval, b, Us);

#endif

  auto result = ZeroScore;

  // Pass 1.
  result += phe->eval();
  result += eval_pieces<KNIGHT, WHITE>(eval) - eval_pieces<KNIGHT, BLACK>(eval);
  result += eval_pieces<BISHOP, WHITE>(eval) - eval_pieces<BISHOP, BLACK>(eval);
  result += eval_pieces<ROOK, WHITE>(eval) - eval_pieces<ROOK, BLACK>(eval);
  result += eval_pieces<QUEEN, WHITE>(eval) - eval_pieces<QUEEN, BLACK>(eval);
  result += eval_king<WHITE>(eval) - eval_king<BLACK>(eval);

  // Pass 2.
  result += eval_passed_pawns<WHITE>(eval) - eval_passed_pawns<BLACK>(eval);

  eval_king_attack<WHITE>(eval);
  eval_king_attack<BLACK>(eval);

  // finally add the remaining poseval scores
  result += eval->poseval[WHITE] - eval->poseval[BLACK];

  if constexpr (Us == WHITE)
    eval->posistion_value[Us] += params::tempo;

  const Stages stageScores = stages(b->material());
  const i32 pos_eval_mg    = static_cast<i32>(result.mg() * stageScores.mg);
  const i32 pos_eval_eg    = static_cast<i32>(result.eg() * stageScores.eg);
  const i32 pos_eval       = pos_eval_mg + pos_eval_eg + (eval->posistion_value[WHITE] - eval->posistion_value[BLACK]);
  const i32 score          = material::evaluate(b->material(), b->flags(), Us == WHITE ? pos_eval : -pos_eval, b, Us);

  return score;
}

namespace Eval {

i32 evaluate(const Board* b, const std::size_t pool_index, const int alpha, const int beta) {
  EvalD e;

  InitEvalD<false>(&e, b, pool_index);

  if (b->side_to_move() == WHITE)
    return evaluate<WHITE>(&e, alpha, beta);

  return evaluate<BLACK>(&e, alpha, beta);
}

i32 tune(const Board* b, const std::size_t pool_index, const int alpha, const int beta) {
  EvalD e;

  InitEvalD<true>(&e, b, pool_index);

  if (b->side_to_move() == WHITE)
    return evaluate<WHITE>(&e, alpha, beta);

  return evaluate<BLACK>(&e, alpha, beta);
}

}   // namespace Eval

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