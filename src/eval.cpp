#include <optional>
#include <memory>
#include "eval.h"
#include "types.h"
#include "square.h"
#include "game.h"
#include "parameters.h"
#include "bitboard.h"
#include "magic.h"
#include "position.h"
#include "pawnhashtable.h"

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
  void eval_pawns_both_sides();
  template<Color Us>
  void eval_material();
  template<Color Us>
  void eval_pawns();
  template<Color Us>
  void eval_knights();
  template<Color Us>
  void eval_bishops();
  template<Color Us>
  void eval_rooks();
  template<Color Us>
  void eval_queens();
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
  PawnHashEntry* pawnp{};

  std::array<int, COL_NB> poseval_mg{};
  std::array<int, COL_NB> poseval_eg{};
  std::array<int, COL_NB> poseval{};
  std::array<int, COL_NB> pawn_eval_mg{};
  std::array<int, COL_NB> pawn_eval_eg{};
  std::array<int, COL_NB> passed_pawn_files{};
  std::array<int, COL_NB> attack_counter{};
  std::array<int, COL_NB> attack_count{};

  std::array<Bitboard, COL_NB> pawn_attacks{};
  std::array<Bitboard, COL_NB> all_attacks{};
  std::array<Bitboard, COL_NB> _knight_attacks{};
  std::array<Bitboard, COL_NB> bishop_attacks{};
  std::array<Bitboard, COL_NB> rook_attacks{};
  std::array<Bitboard, COL_NB> queen_attacks{};
  std::array<Bitboard, COL_NB> king_area{};
  Bitboard open_files{};
  std::array<Bitboard, COL_NB> half_open_files{};
};

template<bool Tuning>
template<Color Us>
int Evaluate<Tuning>::evaluate(const int alpha, const int beta) {

  init_evaluate<WHITE>();

  eval_material<WHITE>();
  eval_material<BLACK>();

#if !defined(NO_EVAL_LAZY_THRESHOLD)

  const auto mat_eval = poseval[WHITE] - poseval[BLACK];

  if (const auto lazy_eval = pos->side_to_move == WHITE ? mat_eval : -mat_eval; lazy_eval - lazy_margin > beta || lazy_eval + lazy_margin < alpha)
    return pos->material.evaluate(pos->flags, lazy_eval, pos->side_to_move, &b);

#endif

  // Pass 1.
  eval_pawns_both_sides();
  eval_knights<WHITE>();
  eval_knights<BLACK>();
  eval_bishops<WHITE>();
  eval_bishops<BLACK>();
  eval_rooks<WHITE>();
  eval_rooks<BLACK>();
  eval_queens<WHITE>();
  eval_queens<BLACK>();
  eval_king<WHITE>();
  eval_king<BLACK>();

  // Pass 2.
  eval_passed_pawns<WHITE>();
  eval_passed_pawns<BLACK>();
  eval_king_attack<WHITE>();
  eval_king_attack<BLACK>();

  const auto stage = (pos->material.value() - pos->material.pawn_value()) / static_cast<double>(Material::max_value_without_pawns);

  poseval[pos->side_to_move] += tempo;

  const auto pos_eval_mg = static_cast<int>((poseval_mg[WHITE] - poseval_mg[BLACK]) * stage);
  const auto pos_eval_eg = static_cast<int>((poseval_eg[WHITE] - poseval_eg[BLACK]) * (1 - stage));
  const auto pos_eval    = pos_eval_mg + pos_eval_eg + (poseval[WHITE] - poseval[BLACK]);
  const auto eval        = pos_eval;

  return pos->material.evaluate(pos->flags, pos->side_to_move == BLACK ? -eval : eval, pos->side_to_move, &b);
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
      pawnp = pawnt->insert(pos->pawn_structure_key, pawn_eval_mg[WHITE] - pawn_eval_mg[BLACK], pawn_eval_eg[WHITE] - pawn_eval_eg[BLACK], passed_pawn_files);
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
    auto bishops = b.bishops(Us);
    add = is_opposite_colors(pop_lsb(&bishops), pop_lsb(&bishops));
  }
  else if (bishop_count > 2) // edge case with more than two bishops
  {
    auto bishops = b.bishops(Us);
    std::array<bool, COL_NB> cols{};
    while (bishops)
    {
      const auto sq = pop_lsb(&bishops);
      cols[color_of(sq)] = true;
    }

    add = cols[WHITE] & cols[BLACK];
  }

  if (add)
  {
    poseval_mg[Us] += bishop_pair_mg;
    poseval_eg[Us] += bishop_pair_eg;
  }

}

template<bool Tuning>
template<Color Us>
void Evaluate<Tuning>::eval_pawns() {
  constexpr auto Them = ~Us;
  auto score_mg       = 0;
  auto score_eg       = 0;

  auto bb = b.pawns(Us);

  while (bb)
  {
    const auto sq     = lsb(bb);
    const auto file   = file_of(sq);
    const auto flipsq = flip[Us][sq];

    if (b.is_pawn_passed(sq, Us))
      passed_pawn_files[Us] |= 1 << file;

    const auto open_file = !b.is_piece_on_file(Pawn, sq, Them);

    if (b.is_pawn_isolated(sq, Us))
    {
      score_mg += pawn_isolated_mg[open_file];
      score_eg += pawn_isolated_eg[open_file];
    } else if (b.is_pawn_behind(sq, Us))
    {
      score_mg += pawn_behind_mg[open_file];
      score_eg += pawn_behind_eg[open_file];
    }
    reset_lsb(bb);

    if (bb & file)
    {
      score_mg += pawn_doubled_mg[open_file];
      score_eg += pawn_doubled_eg[open_file];
    }
    score_mg += pawn_pst_mg[flipsq];
    score_eg += pawn_pst_eg[flipsq];
  }
  pawn_eval_mg[Us] += score_mg;
  pawn_eval_eg[Us] += score_eg;
}

template<bool Tuning>
template<Color Us>
void Evaluate<Tuning>::eval_knights() {
  constexpr auto Them = ~Us;
  auto score_mg       = 0;
  auto score_eg       = 0;
  auto score          = 0;

  auto knights = b.knights(Us);

  while (knights)
  {
    const auto sq     = pop_lsb(&knights);
    const auto flipsq = flip[Us][sq];

    score_mg += knight_pst_mg[flipsq];
    score_eg += knight_pst_eg[flipsq];

    const auto attacks = knight_attacks[sq];

    all_attacks[Us] |= attacks;
    _knight_attacks[Us] |= attacks;

    const auto free_squares = attacks & ~b.occupied_by_side[Us];
    const auto mob          = pop_count(free_squares);

    score_mg += knight_mob_mg[mob];
    score_eg += knight_mob_eg[mob];

    const auto not_defended_by_pawns = pop_count(free_squares & ~pawn_attacks[Them]);
    score_mg += knight_mob2_mg[not_defended_by_pawns];
    score_eg += knight_mob2_eg[not_defended_by_pawns];

    if (const auto attacks_on_king = attacks & king_area[Them]; attacks_on_king)
    {
      attack_counter[Us] += pop_count(attacks_on_king) * knight_attack_king;
      ++attack_count[Us];
    }

    if (pawn_attacks[Them] & sq)
      score += knight_in_danger;
  }

  poseval[Us] += score;
  poseval_mg[Us] += score_mg;
  poseval_eg[Us] += score_eg;
}

template<bool Tuning>
template<Color Us>
void Evaluate<Tuning>::eval_bishops() {
  constexpr auto Them = ~Us;
  auto score_mg       = 0;
  auto score_eg       = 0;
  auto score          = 0;

  auto bishops = b.bishops(Us);

  while (bishops)
  {
    const auto sq     = pop_lsb(&bishops);
    const auto flipsq = flip[Us][sq];

    score_mg += bishop_pst_mg[flipsq];
    score_eg += bishop_pst_eg[flipsq];

    const auto attacks = piece_attacks_bb<Bishop>(sq, b.occupied ^ b.queens(Them));

    all_attacks[Us] |= attacks;
    bishop_attacks[Us] |= attacks;

    const auto free_squares = attacks & ~(b.occupied_by_side[Us]);
    const int mob           = pop_count(free_squares);

    score_mg += bishop_mob_mg[mob];
    score_eg += bishop_mob_eg[mob];

    const int not_defended_by_pawns = pop_count(free_squares & ~pawn_attacks[Them]);

    score_mg += bishop_mob2_mg[not_defended_by_pawns];
    score_eg += bishop_mob2_eg[not_defended_by_pawns];

    if (const auto attacks_on_king = attacks & king_area[Them]; attacks_on_king)
    {
      attack_counter[Us] += pop_count(attacks_on_king) * bishop_attack_king;
      ++attack_count[Us];
    }

    if (pawn_attacks[Them] & sq)
      score += bishop_in_danger;
  }
  poseval[Us] += score;
  poseval_mg[Us] += score_mg;
  poseval_eg[Us] += score_eg;
}

template<bool Tuning>
template<Color Us>
void Evaluate<Tuning>::eval_rooks() {
  constexpr auto Them = ~Us;
  auto score_mg       = 0;
  auto score_eg       = 0;
  auto score          = 0;

  auto rooks = b.rooks(Us);

  while (rooks)
  {
    const auto sq     = pop_lsb(&rooks);
    const auto flipsq = flip[Us][sq];

    score_mg += rook_pst_mg[flipsq];
    score_eg += rook_pst_eg[flipsq];

    if (open_files & sq)
      score += rook_open_file;

    const auto attacks = piece_attacks_bb<Rook>(sq, b.occupied ^ b.queens(Them) ^ b.rooks(Them));

    all_attacks[Us] |= attacks;
    rook_attacks[Us] |= attacks;

    const auto mob = pop_count(attacks & ~b.occupied_by_side[Us]);

    score_mg += rook_mob_mg[mob];
    score_eg += rook_mob_eg[mob];

    if (const auto attacks_on_king = attacks & king_area[Them]; attacks_on_king)
    {
      attack_counter[Us] += pop_count(attacks_on_king) * rook_attack_king;
      ++attack_count[Us];
    }

    if ((pawn_attacks[Them] | _knight_attacks[Them] | bishop_attacks[Them]) & sq)
      score += rook_in_danger;
  }
  poseval[Us] += score;
  poseval_mg[Us] += score_mg;
  poseval_eg[Us] += score_eg;
}

template<bool Tuning>
template<Color Us>
void Evaluate<Tuning>::eval_queens() {
  constexpr auto Them = ~Us;
  auto score_mg       = 0;
  auto score_eg       = 0;
  auto score          = 0;

  for (auto queens = b.queens(Us); queens; reset_lsb(queens))
  {
    const auto sq     = lsb(queens);
    const auto flipsq = flip[Us][sq];

    score_mg += queen_pst_mg[flipsq];
    score_eg += queen_pst_eg[flipsq];

    const auto attacks = piece_attacks_bb<Queen>(sq, b.occupied);

    all_attacks[Us] |= attacks;
    queen_attacks[Us] |= attacks;

    const auto mob = pop_count(attacks & ~b.occupied_by_side[Us]);

    score_mg += queen_mob_mg[mob];
    score_eg += queen_mob_eg[mob];

    if (const auto attacks_on_king = attacks & king_area[Them]; attacks_on_king)
    {
      attack_counter[Us] += pop_count(attacks_on_king) * queen_attack_king;
      ++attack_count[Us];
    }

    if ((pawn_attacks[Them] | _knight_attacks[Them] | bishop_attacks[Them] | rook_attacks[Them]) & sq)
      score += queen_in_danger;
  }
  poseval[Us] += score;
  poseval_mg[Us] += score_mg;
  poseval_eg[Us] += score_eg;
}

template<bool Tuning>
template<Color Us>
void Evaluate<Tuning>::eval_king() {
  constexpr Direction Up = Us == WHITE ? NORTH : SOUTH;
  const auto sq          = lsb(b.king(Us));
  const auto bbsq        = bit(sq);
  const auto flipsq      = flip[Us][sq];

  auto score_mg       = king_pst_mg[flipsq];
  const auto score_eg = king_pst_eg[flipsq];

  score_mg += king_pawn_shelter[pop_count((pawn_push<Up>(bbsq) | pawn_west_attacks[Us](bbsq) | pawn_east_attacks[Us](bbsq)) & b.pawns(Us))];

  const auto eastwest = bbsq | west_one(bbsq) | east_one(bbsq);

  score_mg += king_on_open[pop_count(open_files & eastwest)];
  score_mg += king_on_half_open[pop_count(half_open_files[Us] & eastwest)];

  if (((Us == 0) && (((sq == f1 || sq == g1) && (b.rooks(WHITE) & h1)) || ((sq == c1 || sq == b1) && (b.rooks(WHITE) & a1))))
      || ((Us == 1) && (((sq == f8 || sq == g8) && (b.rooks(BLACK) & h8)) || ((sq == c8 || sq == b8) && (b.rooks(BLACK) & a8)))))
    score_mg += king_obstructs_rook;

  all_attacks[Us] |= king_attacks[b.king_square[Us]];
  poseval_mg[Us] += score_mg;
  poseval_eg[Us] += score_eg;
}

template<bool Tuning>
template<Color Us>
void Evaluate<Tuning>::eval_passed_pawns() {
  constexpr auto Them = ~Us;

  if (pawnp == nullptr)
    return;

  for (uint64_t files = pawnp->passed_pawn_files[Us]; files; reset_lsb(files))
  {
    for (auto bb = bb_file(lsb(files)) & b.pawns(Us); bb; reset_lsb(bb))
    {
      const auto sq         = lsb(bb);
      const auto front_span = pawn_front_span[Us][sq];
      const auto r          = relative_rank(Us, sq);
      const auto score_mg   = passed_pawn_mg[r];
      auto score_eg         = passed_pawn_eg[r];

      score_eg += passed_pawn_no_us[r] * (front_span & b.occupied_by_side[Us] ? 0 : 1);
      score_eg += passed_pawn_no_them[r] * (front_span & b.occupied_by_side[Them] ? 0 : 1);
      score_eg += passed_pawn_no_attacks[r] * (front_span & all_attacks[Them] ? 0 : 1);
      score_eg += passed_pawn_king_dist_them[dist[sq][b.king_square[Them]]];
      score_eg += passed_pawn_king_dist_us[dist[sq][b.king_square[Us]]];

      poseval_mg[Us] += score_mg;
      poseval_eg[Us] += score_eg;
    }
  }
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
  pos->flags = 0;

  poseval_mg.fill(0);
  poseval_eg.fill(0);
  poseval.fill(0);

  attack_count.fill(0);
  attack_counter.fill(0);

  king_area[WHITE] = king_attacks[b.king_square[WHITE]] | b.king_square[WHITE];
  king_area[BLACK] = king_attacks[b.king_square[BLACK]] | b.king_square[BLACK];

  const auto white_pawns = b.pawns(WHITE);
  const auto black_pawns = b.pawns(BLACK);

  open_files             = ~(north_fill(south_fill(white_pawns)) | north_fill(south_fill(black_pawns)));
  half_open_files[WHITE] = ~north_fill(south_fill(white_pawns)) & ~open_files;
  half_open_files[BLACK] = ~north_fill(south_fill(black_pawns)) & ~open_files;

  all_attacks[WHITE] = pawn_attacks[WHITE] = pawn_east_attacks[WHITE](white_pawns) | pawn_west_attacks[WHITE](white_pawns);
  all_attacks[BLACK] = pawn_attacks[BLACK] = pawn_east_attacks[BLACK](black_pawns) | pawn_west_attacks[BLACK](black_pawns);

  _knight_attacks.fill(0);
  bishop_attacks.fill(0);
  rook_attacks.fill(0);
  queen_attacks.fill(0);
}

namespace Eval {

int evaluate(Game *g, PawnHashTable *pawnTable, const int alpha, const int beta) {
  return g->pos->side_to_move == WHITE ? Evaluate<false>(g, pawnTable).evaluate<WHITE>(alpha, beta) : Evaluate<false>(g, pawnTable).evaluate<BLACK>(alpha, beta);
}

int tune(Game *g, PawnHashTable *pawnTable, const int alpha, const int beta) {
  return g->pos->side_to_move == WHITE ? Evaluate<true>(g, pawnTable).evaluate<WHITE>(alpha, beta) : Evaluate<true>(g, pawnTable).evaluate<BLACK>(alpha, beta);
}


}// namespace Eval