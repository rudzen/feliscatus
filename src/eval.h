#pragma once

#include <array>
#include "game.h"
#include "hash.h"
#include "position.h"
#include "parameters.h"

class Eval {
public:
  Eval(const Game &game, PawnHashTable *pawntable) : game_(game), pawnt(pawntable), tuning_(false) {}

  virtual ~Eval() = default;

  int evaluate(const int alpha, const int beta) {

    constexpr auto lazy_margin = 500;

    init_evaluate();

    eval_material<WHITE>();
    eval_material<BLACK>();

    const auto mat_eval    = poseval[WHITE] - poseval[BLACK];

    if (const auto lazy_eval = pos->side_to_move == WHITE ? mat_eval : -mat_eval; lazy_eval - lazy_margin > beta || lazy_eval + lazy_margin < alpha)
      return pos->material.evaluate(pos->flags, lazy_eval, pos->side_to_move, &game_.board);

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

    poseval[pos->side_to_move] += 10;

    const auto pos_eval_mg = static_cast<int>((poseval_mg[WHITE] - poseval_mg[BLACK]) * stage);
    const auto pos_eval_eg = static_cast<int>((poseval_eg[WHITE] - poseval_eg[BLACK]) * (1 - stage));
    const auto pos_eval    = pos_eval_mg + pos_eval_eg + (poseval[WHITE] - poseval[BLACK]);
    const auto eval        = pos_eval;

    return pos->material.evaluate(pos->flags, pos->side_to_move == BLACK ? -eval : eval, pos->side_to_move, &game_.board);
  }

protected:
  void eval_pawns_both_sides() {
    pawnp = nullptr;

    if (pos->material.pawn_count())
    {
      pawnp = tuning_ ? nullptr : pawnt->find(pos->pawn_structure_key);

      if (!pawnp)
      {
        pawn_eval_mg.fill(0);
        pawn_eval_eg.fill(0);

        passed_pawn_files.fill(0);

        eval_pawns<WHITE>();
        eval_pawns<BLACK>();

        pawnp = pawnt->insert(pos->pawn_structure_key,  pawn_eval_mg[WHITE] - pawn_eval_mg[BLACK], pawn_eval_eg[WHITE] - pawn_eval_eg[BLACK], passed_pawn_files);
      }
      poseval_mg[0] += pawnp->eval_mg;
      poseval_eg[0] += pawnp->eval_eg;
    }
  }

  template<Color Us>
  void eval_pawns() {

    constexpr auto Them = ~Us;
    auto score_mg       = 0;
    auto score_eg       = 0;

    for (auto bb = pawns(Us); bb;)
    {
      const auto sq = lsb(bb);

      if (game_.board.is_pawn_passed(sq, Us))
        passed_pawn_files[Us] |= 1 << file_of(sq);

      const auto open_file = !game_.board.is_piece_on_file(Pawn, sq, Them) ? 1 : 0;

      if (game_.board.is_pawn_isolated(sq, Us))
      {
        score_mg += pawn_isolated_mg[open_file];
        score_eg += pawn_isolated_eg[open_file];
      } else if (game_.board.is_pawn_behind(sq, Us))
      {
        score_mg += pawn_behind_mg[open_file];
        score_eg += pawn_behind_eg[open_file];
      }
      reset_lsb(bb);

      if (bb_file(sq) & bb)
      {
        score_mg += pawn_doubled_mg[open_file];
        score_eg += pawn_doubled_eg[open_file];
      }
      score_mg += pawn_pst_mg[flip[Us][sq]];
      score_eg += pawn_pst_eg[flip[Us][sq]];
    }
    pawn_eval_mg[Us] += score_mg;
    pawn_eval_eg[Us] += score_eg;
  }

  template<Color Us>
  void eval_knights() {
    constexpr auto Them = ~Us;
    auto score_mg       = 0;
    auto score_eg       = 0;
    auto score          = 0;

    for (auto knights = game_.board.knights(Us); knights; reset_lsb(knights))
    {
      const Square sq       = lsb(knights);
      const uint64_t flipsq = flip[Us][sq];

      score_mg += knight_pst_mg[flipsq];
      score_eg += knight_pst_eg[flipsq];

      const auto &attacks = knight_attacks[sq];
      int x               = pop_count(attacks & ~game_.board.occupied_by_side[Us]);

      score_mg += knight_mob_mg[x];
      score_eg += knight_mob_eg[x];

      x = pop_count(attacks & ~game_.board.occupied_by_side[Us] & ~pawn_attacks[Them]);
      score_mg += knight_mob2_mg[x];
      score_eg += knight_mob2_eg[x];

      all_attacks[Us] |= attacks;
      _knight_attacks[Us] |= attacks;

      if (attacks & king_area[Them])
      {
        attack_counter[Us] += pop_count(attacks & king_area[Them]) * knight_attack_king;
        ++attack_count[Us];
      }

      if (pawn_attacks[Them] & sq)
        score += knight_in_danger;
    }

    poseval[Us] += score;
    poseval_mg[Us] += score_mg;
    poseval_eg[Us] += score_eg;
  }

  template<Color Us>
  void eval_bishops() {
    constexpr auto Them = ~Us;
    auto score_mg       = 0;
    auto score_eg       = 0;
    auto score          = 0;

    for (auto bishops = game_.board.bishops(Us); bishops; reset_lsb(bishops))
    {
      const auto sq     = lsb(bishops);
      const auto flipsq = flip[Us][sq];

      score_mg += bishop_pst_mg[flipsq];
      score_eg += bishop_pst_eg[flipsq];

      const auto attacks = bishopAttacks(sq, occupied);
      const int x        = pop_count(attacks & ~(game_.board.occupied_by_side[Us]));

      score_mg += bishop_mob_mg[x];
      score_eg += bishop_mob_eg[x];

      const int x1 = pop_count(attacks & ~game_.board.occupied_by_side[Us] & ~pawn_attacks[Them]);
      score_mg += bishop_mob2_mg[x1];
      score_eg += bishop_mob2_eg[x1];

      all_attacks[Us] |= attacks;
      bishop_attacks[Us] |= attacks;

      if (attacks & king_area[Them])
      {
        attack_counter[Us] += pop_count(attacks & king_area[Them]) * bishop_attack_king;
        ++attack_count[Us];
      }

      if (pawn_attacks[Them] & sq)
        score += bishop_in_danger;
    }
    poseval[Us] += score;
    poseval_mg[Us] += score_mg;
    poseval_eg[Us] += score_eg;
  }

  template<Color Us>
  void eval_rooks() {
    constexpr auto Them = ~Us;
    auto score_mg       = 0;
    auto score_eg       = 0;
    auto score          = 0;

    for (auto rooks = game_.board.rooks(Us); rooks; reset_lsb(rooks))
    {
      const auto sq     = lsb(rooks);
      const auto flipsq = flip[Us][sq];

      score_mg += rook_pst_mg[flipsq];
      score_eg += rook_pst_eg[flipsq];

      if (open_files & sq)
        score += rook_open_file;

      const auto attacks = rookAttacks(sq, occupied);
      const int x        = pop_count(attacks & ~game_.board.occupied_by_side[Us]);

      score_mg += rook_mob_mg[x];
      score_eg += rook_mob_eg[x];

      all_attacks[Us] |= attacks;
      rook_attacks[Us] |= attacks;

      if (attacks & king_area[Them])
      {
        attack_counter[Us] += pop_count(attacks & king_area[Them]) * rook_attack_king;
        ++attack_count[Us];
      }

      if ((pawn_attacks[Them] | _knight_attacks[Them] | bishop_attacks[Them]) & sq)
        score += rook_in_danger;
    }
    poseval[Us] += score;
    poseval_mg[Us] += score_mg;
    poseval_eg[Us] += score_eg;
  }

  template<Color Us>
  void eval_queens() {
    constexpr auto Them = ~Us;
    auto score_mg       = 0;
    auto score_eg       = 0;
    auto score          = 0;

    for (auto queens = game_.board.queens(Us); queens; reset_lsb(queens))
    {
      const auto sq     = lsb(queens);
      const auto flipsq = flip[Us][sq];

      score_mg += queen_pst_mg[flipsq];
      score_eg += queen_pst_eg[flipsq];

      const auto attacks = queenAttacks(sq, occupied);
      const int x        = pop_count(attacks & ~game_.board.occupied_by_side[Us]);

      score_mg += queen_mob_mg[x];
      score_eg += queen_mob_eg[x];

      all_attacks[Us] |= attacks;
      queen_attacks[Us] |= attacks;

      if (attacks & king_area[Them])
      {
        attack_counter[Us] += pop_count(attacks & king_area[Them]) * queen_attack_king;
        ++attack_count[Us];
      }

      if ((pawn_attacks[Them] | _knight_attacks[Them] | bishop_attacks[Them] | rook_attacks[Them]) & sq)
        score += queen_in_danger;
    }
    poseval[Us] += score;
    poseval_mg[Us] += score_mg;
    poseval_eg[Us] += score_eg;
  }

  template<Color Us>
  void eval_material() {
    poseval[Us] = pos->material.material_value[Us];

    if (pos->material.count(Us, Bishop) == 2)
    {
      poseval_mg[Us] += bishop_pair_mg;
      poseval_eg[Us] += bishop_pair_eg;
    }
  }

  template<Color Us>
  void eval_king() {
    constexpr Direction Up = Us == WHITE ? NORTH : SOUTH;
    const auto sq    = lsb(game_.board.king(Us));
    const auto bbsq  = bit(sq);

    auto score_mg       = king_pst_mg[flip[Us][sq]];
    const auto score_eg = king_pst_eg[flip[Us][sq]];

    score_mg += king_pawn_shelter[pop_count((pawn_push<Up>(bbsq) | pawn_west_attacks[Us](bbsq) | pawn_east_attacks[Us](bbsq)) & pawns(Us))];

    const auto eastwest = bbsq | west_one(bbsq) | east_one(bbsq);

    score_mg += king_on_open[pop_count(open_files & eastwest)];
    score_mg += king_on_half_open[pop_count(half_open_files[Us] & eastwest)];

    if (((Us == 0) && (((sq == f1 || sq == g1) && (game_.board.rooks(WHITE) & h1)) || ((sq == c1 || sq == b1) && (game_.board.rooks(WHITE) & a1))))
        || ((Us == 1) && (((sq == f8 || sq == g8) && (game_.board.rooks(BLACK) & h8)) || ((sq == c8 || sq == b8) && (game_.board.rooks(BLACK) & a8)))))
      score_mg += king_obstructs_rook;

    all_attacks[Us] |= king_attacks[king_sq(Us)];
    poseval_mg[Us] += score_mg;
    poseval_eg[Us] += score_eg;
  }

  template<Color Us>
  void eval_passed_pawns() {
    constexpr const auto Them = ~Us;

    for (uint64_t files = pawnp ? pawnp->passed_pawn_files[Us] : 0; files; reset_lsb(files))
    {
      for (auto bb = bb_file(lsb(files)) & pawns(Us); bb; reset_lsb(bb))
      {
        const auto sq          = lsb(bb);
        const auto front_span  = pawn_front_span[Us][sq];
        const auto r           = relative_rank(Us, sq);

        const auto score_mg = passed_pawn_mg[r];
        auto score_eg       = passed_pawn_eg[r];

        score_eg += passed_pawn_no_us[r] * (front_span & game_.board.occupied_by_side[Us] ? 0 : 1);
        score_eg += passed_pawn_no_them[r] * (front_span & game_.board.occupied_by_side[Them] ? 0 : 1);
        score_eg += passed_pawn_no_attacks[r] * (front_span & all_attacks[Them] ? 0 : 1);
        score_eg += passed_pawn_king_dist_them[dist[sq][king_sq(Them)]];
        score_eg += passed_pawn_king_dist_us[dist[sq][king_sq(Us)]];

        poseval_mg[Us] += score_mg;
        poseval_eg[Us] += score_eg;
      }
    }
  }

  template<Color Us>
  void eval_king_attack() {
    if (attack_count[Us] > 1)
      poseval_mg[Us] += attack_counter[Us] * (attack_count[Us] - 1);
  }

  [[nodiscard]]
  Bitboard pawns(const Color side) const { return game_.board.pawns(side); }

  [[nodiscard]]
  Square king_sq(const Color side) const { return game_.board.king_square[side]; }

  void init_evaluate() {
    pos        = game_.pos;
    pos->flags = 0;

    poseval_mg.fill(0);
    poseval_eg.fill(0);
    poseval.fill(0);

    attack_count.fill(0);
    attack_counter.fill(0);

    king_area[0] = king_attacks[king_sq(WHITE)] | game_.board.king(WHITE);
    king_area[1] = king_attacks[king_sq(BLACK)] | game_.board.king(BLACK);

    occupied     = game_.board.occupied;
    not_occupied = ~occupied;

    const auto white_pawns = pawns(WHITE);
    const auto black_pawns = pawns(BLACK);

    open_files         = ~(north_fill(south_fill(white_pawns)) | north_fill(south_fill(black_pawns)));
    half_open_files[0] = ~north_fill(south_fill(white_pawns)) & ~open_files;
    half_open_files[1] = ~north_fill(south_fill(black_pawns)) & ~open_files;

    all_attacks[0] = pawn_attacks[0] = pawn_east_attacks[0](white_pawns) | pawn_west_attacks[0](white_pawns);
    all_attacks[1] = pawn_attacks[1] = pawn_east_attacks[1](black_pawns) | pawn_west_attacks[1](black_pawns);

    _knight_attacks.fill(0);
    bishop_attacks.fill(0);
    rook_attacks.fill(0);
    queen_attacks.fill(0);
  }

  Position *pos{};
  const Game &game_{};
  PawnHashTable *pawnt{};
  PawnHashEntry *pawnp{};

  std::array<int, 2> poseval_mg{};
  std::array<int, 2> poseval_eg{};
  std::array<int, 2> poseval{};
  std::array<int, 2> pawn_eval_mg{};
  std::array<int, 2> pawn_eval_eg{};
  std::array<int, 2> passed_pawn_files{};
  std::array<int, 2> attack_counter{};
  std::array<int, 2> attack_count{};

  std::array<Bitboard, 2> pawn_attacks{};
  std::array<Bitboard, 2> all_attacks{};
  std::array<Bitboard, 2> _knight_attacks{};
  std::array<Bitboard, 2> bishop_attacks{};
  std::array<Bitboard, 2> rook_attacks{};
  std::array<Bitboard, 2> queen_attacks{};
  std::array<Bitboard, 2> king_area{};
  Bitboard occupied{};
  Bitboard not_occupied{};
  Bitboard open_files{};
  std::array<Bitboard, 2> half_open_files{};
public:
  bool tuning_;
};
