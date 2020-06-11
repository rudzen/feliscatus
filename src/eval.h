#pragma once

#include <array>
#include "game.h"
#include "hash.h"

class Eval {
public:
  Eval(const Game &game, PawnHashTable *pawntable) : game_(game), pawnt(pawntable), tuning_(false) { }

  virtual ~Eval() {}

  int evaluate(int alpha, int beta) {
    initEvaluate();

    evalMaterial<0>();
    evalMaterial<1>();

    const int mat_eval = poseval[0] - poseval[1];
    const int lazy_margin = 500;

    if (const int lazy_eval   = pos->side_to_move == 0 ? mat_eval : -mat_eval; lazy_eval - lazy_margin > beta || lazy_eval + lazy_margin < alpha)
      return pos->material.evaluate(pos->flags, lazy_eval, pos->side_to_move, &game_.board);

    // Pass 1.
    evalPawnsBothSides();
    evalKnights<0>();
    evalKnights<1>();
    evalBishops<0>();
    evalBishops<1>();
    evalRooks<0>();
    evalRooks<1>();
    evalQueens<0>();
    evalQueens<1>();
    evalKing<0>();
    evalKing<1>();

    // Pass 2.
    evalPassedPawns<0>();
    evalPassedPawns<1>();
    evalKingAttack<0>();
    evalKingAttack<1>();

    const double stage = (pos->material.value() - pos->material.pawnValue()) / static_cast<double>(pos->material.max_value_without_pawns);

    poseval[pos->side_to_move] += 10;

    int pos_eval_mg = static_cast<int>((poseval_mg[0] - poseval_mg[1]) * stage);
    int pos_eval_eg = static_cast<int>((poseval_eg[0] - poseval_eg[1]) * (1 - stage));
    int pos_eval    = pos_eval_mg + pos_eval_eg + (poseval[0] - poseval[1]);
    int eval        = pos_eval;

    return pos->material.evaluate(pos->flags, pos->side_to_move == 1 ? -eval : eval, pos->side_to_move, &game_.board);
  }

protected:
  void evalPawnsBothSides() {
    pawnp = 0;

    if (pos->material.pawnCount())
    {
      pawnp = tuning_ ? 0 : pawnt->find(pos->pawn_structure_key);

      if (!pawnp)
      {
        pawn_eval_mg[0] = pawn_eval_mg[1] = 0;
        pawn_eval_eg[0] = pawn_eval_eg[1] = 0;

        passed_pawn_files[0] = passed_pawn_files[1] = 0;

        evalPawns<0>();
        evalPawns<1>();

        pawnp = pawnt->insert(pos->pawn_structure_key, (int)(pawn_eval_mg[0] - pawn_eval_mg[1]), (int)(pawn_eval_eg[0] - pawn_eval_eg[1]), passed_pawn_files);
      }
      poseval_mg[0] += pawnp->eval_mg;
      poseval_eg[0] += pawnp->eval_eg;
    }
  }

  template<int Us>
  void evalPawns() {
    int score_mg = 0;
    int score_eg = 0;

    for (uint64_t bb = pawns(Us); bb;)
    {
      uint64_t sq = lsb(bb);

      if (game_.board.isPawnPassed(sq, Us))
        passed_pawn_files[Us] |= 1 << file_of(sq);

      const int open_file = !game_.board.isPieceOnFile(Pawn, sq, Us ^ 1) ? 1 : 0;

      if (game_.board.isPawnIsolated(sq, Us))
      {
        score_mg += pawn_isolated_mg[open_file];
        score_eg += pawn_isolated_eg[open_file];
      } else if (game_.board.isPawnBehind(sq, Us))
      {
        score_mg += pawn_behind_mg[open_file];
        score_eg += pawn_behind_eg[open_file];
      }
      resetLSB(bb);

      if (bbFile(sq) & bb)
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

  template<int Us>
  void evalKnights() {
    constexpr int Them = Us ^ 1;
    int score_mg       = 0;
    int score_eg       = 0;
    int score          = 0;

    for (uint64_t knights = game_.board.knights(Us); knights; resetLSB(knights))
    {
      uint64_t sq     = lsb(knights);
      uint64_t flipsq = flip[Us][sq];

      score_mg += knight_pst_mg[flipsq];
      score_eg += knight_pst_eg[flipsq];

      const uint64_t &attacks = knight_attacks[sq];
      int x                   = popCount(attacks & ~game_.board.occupied_by_side[Us]);

      score_mg += knight_mob_mg[x];
      score_eg += knight_mob_eg[x];

      x = popCount(attacks & ~game_.board.occupied_by_side[Us] & ~pawn_attacks[Them]);
      score_mg += knight_mob2_mg[x];
      score_eg += knight_mob2_eg[x];

      all_attacks[Us] |= attacks;
      _knight_attacks[Us] |= attacks;

      if (attacks & king_area[Them])
      {
        attack_counter[Us] += popCount(attacks & king_area[Them]) * knight_attack_king;
        attack_count[Us]++;
      }

      if (bbSquare(sq) & pawn_attacks[Them])
        score += knight_in_danger;
    }

    poseval[Us] += score;
    poseval_mg[Us] += score_mg;
    poseval_eg[Us] += score_eg;
  }

  template<int Us>
  void evalBishops() {
    constexpr int Them = Us ^ 1;
    int score_mg       = 0;
    int score_eg       = 0;
    int score          = 0;

    for (uint64_t bishops = game_.board.bishops(Us); bishops; resetLSB(bishops))
    {
      uint64_t sq     = lsb(bishops);
      uint64_t flipsq = flip[Us][sq];

      score_mg += bishop_pst_mg[flipsq];
      score_eg += bishop_pst_eg[flipsq];

      const uint64_t attacks = bishopAttacks(sq, occupied);
      int x                  = popCount(attacks & ~(game_.board.occupied_by_side[Us]));

      score_mg += bishop_mob_mg[x];
      score_eg += bishop_mob_eg[x];

      int x1 = popCount(attacks & ~game_.board.occupied_by_side[Us] & ~pawn_attacks[Them]);
      score_mg += bishop_mob2_mg[x1];
      score_eg += bishop_mob2_eg[x1];

      all_attacks[Us] |= attacks;
      bishop_attacks[Us] |= attacks;

      if (attacks & king_area[Them])
      {
        attack_counter[Us] += popCount(attacks & king_area[Them]) * bishop_attack_king;
        attack_count[Us]++;
      }

      if (bbSquare(sq) & pawn_attacks[Them])
        score += bishop_in_danger;
    }
    poseval[Us] += score;
    poseval_mg[Us] += score_mg;
    poseval_eg[Us] += score_eg;
  }

  template<int Us>
  void evalRooks() {
    constexpr int Them = Us ^ 1;
    int score_mg       = 0;
    int score_eg       = 0;
    int score          = 0;

    for (uint64_t rooks = game_.board.rooks(Us); rooks; resetLSB(rooks))
    {
      const uint64_t sq     = lsb(rooks);
      const uint64_t flipsq = flip[Us][sq];

      score_mg += rook_pst_mg[flipsq];
      score_eg += rook_pst_eg[flipsq];

      const uint64_t &bbsq = bbSquare(sq);

      if (bbsq & open_files)
        score += rook_open_file;

      const uint64_t attacks = rookAttacks(sq, occupied);
      const int x            = popCount(attacks & ~game_.board.occupied_by_side[Us]);

      score_mg += rook_mob_mg[x];
      score_eg += rook_mob_eg[x];

      all_attacks[Us] |= attacks;
      rook_attacks[Us] |= attacks;

      if (attacks & king_area[Them])
      {
        attack_counter[Us] += popCount(attacks & king_area[Them]) * rook_attack_king;
        attack_count[Us]++;
      }

      if (bbSquare(sq) & (pawn_attacks[Them] | _knight_attacks[Them] | bishop_attacks[Them]))
      {
        score += rook_in_danger;
      }
    }
    poseval[Us] += score;
    poseval_mg[Us] += score_mg;
    poseval_eg[Us] += score_eg;
  }

  template<int Us>
  void evalQueens() {
    const int Them = Us ^ 1;
    int score_mg   = 0;
    int score_eg   = 0;
    int score      = 0;

    for (uint64_t queens = game_.board.queens(Us); queens; resetLSB(queens))
    {
      uint64_t sq     = lsb(queens);
      uint64_t flipsq = flip[Us][sq];

      score_mg += queen_pst_mg[flipsq];
      score_eg += queen_pst_eg[flipsq];

      const uint64_t attacks = queenAttacks(sq, occupied);
      int x                  = popCount(attacks & ~game_.board.occupied_by_side[Us]);

      score_mg += queen_mob_mg[x];
      score_eg += queen_mob_eg[x];

      all_attacks[Us] |= attacks;
      queen_attacks[Us] |= attacks;

      if (attacks & king_area[Them])
      {
        attack_counter[Us] += popCount(attacks & king_area[Them]) * queen_attack_king;
        attack_count[Us]++;
      }

      if (bbSquare(sq) & (pawn_attacks[Them] | _knight_attacks[Them] | bishop_attacks[Them] | rook_attacks[Them]))
      {
        score += queen_in_danger;
      }
    }
    poseval[Us] += score;
    poseval_mg[Us] += score_mg;
    poseval_eg[Us] += score_eg;
  }

  template<int Us>
  void evalMaterial() {
    poseval[Us] = pos->material.material_value[Us];

    if (pos->material.count(Us, Bishop) == 2)
    {
      poseval_mg[Us] += bishop_pair_mg;
      poseval_eg[Us] += bishop_pair_eg;
    }
  }

  template<int Us>
  void evalKing() {
    uint64_t sq          = lsb(game_.board.king(Us));
    const uint64_t &bbsq = bbSquare(sq);

    int score_mg = king_pst_mg[flip[Us][sq]];
    int score_eg = king_pst_eg[flip[Us][sq]];

    score_mg += king_pawn_shelter[popCount((pawnPush[Us](bbsq) | pawnWestAttacks[Us](bbsq) | pawnEastAttacks[Us](bbsq)) & pawns(Us))];

    uint64_t eastwest = bbsq | westOne(bbsq) | eastOne(bbsq);

    score_mg += king_on_open[popCount(open_files & eastwest)];
    score_mg += king_on_half_open[popCount(half_open_files[Us] & eastwest)];

    if (((Us == 0) && (((sq == f1 || sq == g1) && (bbSquare(h1) & game_.board.rooks(0))) || ((sq == c1 || sq == b1) && (bbSquare(a1) & game_.board.rooks(0)))))
        || ((Us == 1) && (((sq == f8 || sq == g8) && (bbSquare(h8) & game_.board.rooks(1))) || ((sq == c8 || sq == b8) && (bbSquare(a8) & game_.board.rooks(1))))))
    {
      score_mg += king_obstructs_rook;
    }

    all_attacks[Us] |= king_attacks[kingSq(Us)];
    poseval_mg[Us] += score_mg;
    poseval_eg[Us] += score_eg;
  }

  template<int Us>
  void evalPassedPawns() {
    constexpr int Them = Us ^ 1;

    for (uint64_t files = pawnp ? pawnp->passed_pawn_files[Us] : 0; files; resetLSB(files))
    {
      for (uint64_t bb = bbFile(lsb(files)) & pawns(Us); bb; resetLSB(bb))
      {
        int sq                     = lsb(bb);
        const uint64_t &front_span = pawn_front_span[Us][sq];
        int r                      = Us == 0 ? rank_of(sq) : 7 - rank_of(sq);

        int score_mg = passed_pawn_mg[r];
        int score_eg = passed_pawn_eg[r];

        score_eg += passed_pawn_no_us[r] * (front_span & game_.board.occupied_by_side[Us] ? 0 : 1);
        score_eg += passed_pawn_no_them[r] * (front_span & game_.board.occupied_by_side[Them] ? 0 : 1);
        score_eg += passed_pawn_no_attacks[r] * (front_span & all_attacks[Them] ? 0 : 1);
        score_eg += passed_pawn_king_dist_them[dist[sq][kingSq(Them)]];
        score_eg += passed_pawn_king_dist_us[dist[sq][kingSq(Us)]];

        poseval_mg[Us] += score_mg;
        poseval_eg[Us] += score_eg;
      }
    }
  }

  template<int Us>
  void evalKingAttack() {
    if (attack_count[Us] > 1)
      poseval_mg[Us] += attack_counter[Us] * (attack_count[Us] - 1);
  }

  const uint64_t &pawns(int side) { return game_.board.pawns(side); }

  uint64_t kingSq(int side) { return game_.board.king_square[side]; }

  void initEvaluate() {
    pos        = game_.pos;
    pos->flags = 0;

    poseval_mg.fill(0);
    poseval_eg.fill(0);
    poseval.fill(0);

    attack_count.fill(0);
    attack_counter.fill(0);

    king_area[0] = king_attacks[kingSq(0)] | game_.board.king(0);
    king_area[1] = king_attacks[kingSq(1)] | game_.board.king(1);

    occupied     = game_.board.occupied;
    not_occupied = ~occupied;

    const auto whitePawns = pawns(0);
    const auto blackPawns = pawns(1);

    open_files         = ~(northFill(southFill(whitePawns)) | northFill(southFill(blackPawns)));
    half_open_files[0] = ~northFill(southFill(whitePawns)) & ~open_files;
    half_open_files[1] = ~northFill(southFill(blackPawns)) & ~open_files;

    all_attacks[0] = pawn_attacks[0] = pawnEastAttacks[0](whitePawns) | pawnWestAttacks[0](whitePawns);
    all_attacks[1] = pawn_attacks[1] = pawnEastAttacks[1](blackPawns) | pawnWestAttacks[1](blackPawns);

    _knight_attacks.fill(0);
    bishop_attacks.fill(0);
    rook_attacks.fill(0);
    queen_attacks.fill(0);
  }

  Position *pos;
  const Game &game_;
  PawnHashTable *pawnt;
  PawnHashEntry *pawnp;

  std::array<int, 2> poseval_mg{};
  std::array<int, 2> poseval_eg{};
  std::array<int, 2> poseval{};
  std::array<int, 2> pawn_eval_mg{};
  std::array<int, 2> pawn_eval_eg{};
  std::array<int, 2> passed_pawn_files{};
  std::array<int, 2> attack_counter{};
  std::array<int, 2> attack_count{};

  std::array<uint64_t, 2> pawn_attacks{};
  std::array<uint64_t, 2> all_attacks{};
  std::array<uint64_t, 2> _knight_attacks{};
  std::array<uint64_t, 2> bishop_attacks{};
  std::array<uint64_t, 2> rook_attacks{};
  std::array<uint64_t, 2> queen_attacks{};
  std::array<uint64_t, 2> king_area{};
  uint64_t occupied;
  uint64_t not_occupied;
  uint64_t open_files;
  std::array<uint64_t, 2> half_open_files{};

public:
  static int knight_mob_mg[9];
  static int knight_mob_eg[9];
  static int bishop_mob_mg[14];
  static int bishop_mob_eg[14];
  static int rook_mob_mg[15];
  static int rook_mob_eg[15];
  static int queen_mob_mg[28];
  static int queen_mob_eg[28];
  static int knight_mob2_mg[9];
  static int knight_mob2_eg[9];
  static int bishop_mob2_mg[14];
  static int bishop_mob2_eg[14];
  //---------------------------
  static int pawn_pst_mg[64];
  static int pawn_pst_eg[64];
  static int knight_pst_mg[64];
  static int knight_pst_eg[64];
  static int bishop_pst_mg[64];
  static int bishop_pst_eg[64];
  static int rook_pst_mg[64];
  static int rook_pst_eg[64];
  static int queen_pst_mg[64];
  static int queen_pst_eg[64];
  static int king_pst_mg[64];
  static int king_pst_eg[64];
  //---------------------------
  static int pawn_isolated_mg[2];
  static int pawn_isolated_eg[2];
  static int pawn_behind_mg[2];
  static int pawn_behind_eg[2];
  static int pawn_doubled_mg[2];
  static int pawn_doubled_eg[2];
  //---------------------------
  static int passed_pawn_mg[8];
  static int passed_pawn_eg[8];
  static int passed_pawn_no_them[8];
  static int passed_pawn_no_us[8];
  static int passed_pawn_no_attacks[8];
  static int passed_pawn_king_dist_them[8];
  static int passed_pawn_king_dist_us[8];
  //---------------------------
  static int king_pawn_shelter[4];
  static int king_on_open[4];
  static int king_on_half_open[4];
  //---------------------------
  static int bishop_pair_mg;
  static int bishop_pair_eg;
  //---------------------------
  static int king_obstructs_rook;
  //---------------------------
  static int rook_open_file;
  //---------------------------
  static int knight_in_danger;
  static int bishop_in_danger;
  static int rook_in_danger;
  static int queen_in_danger;
  //---------------------------
  static int knight_attack_king;
  static int bishop_attack_king;
  static int rook_attack_king;
  static int queen_attack_king;

  bool tuning_;
};

int Eval::bishop_attack_king = 5;
int Eval::bishop_in_danger   = -56;
int Eval::bishop_mob2_eg[14] = {-60, -27, -20, -10, -8, -2, 2, 3, 4, 1, 6, 10, 16, 3};
int Eval::bishop_mob2_mg[14] = {-27, -14, -6, -1, 3, 7, 5, 7, 11, 15, 10, 18, 12, 29};
int Eval::bishop_mob_eg[14]  = {-35, -20, -29, -20, -13, -6, -4, 0, 5, 8, -1, 5, 7, 2};
int Eval::bishop_mob_mg[14]  = {-23, -17, -13, -13, -6, 0, 5, 5, 10, 3, 2, 6, -14, 16};
int Eval::bishop_pair_eg     = 57;
int Eval::bishop_pair_mg     = 29;

int Eval::king_obstructs_rook  = -49;
int Eval::king_on_half_open[4] = {12, -9, -55, -97};
int Eval::king_on_open[4]      = {29, -13, -63, -221};
int Eval::king_pawn_shelter[4] = {-20, 1, 6, -7};

int Eval::knight_attack_king = 3;
int Eval::knight_in_danger   = -41;
int Eval::knight_mob2_eg[9]  = {-137, -56, -38, -17, -7, 0, 0, -5, -15};
int Eval::knight_mob2_mg[9]  = {-59, -41, -21, -10, 1, 8, 14, 22, 24};
int Eval::knight_mob_eg[9]   = {-40, 8, 14, 0, -5, -9, -10, -15, -8};
int Eval::knight_mob_mg[9]   = {76, 23, 5, 3, 3, -2, -6, -14, -16};

int Eval::passed_pawn_eg[8]             = {0, -16, -7, 2, 12, 13, -115, 0};
int Eval::passed_pawn_king_dist_them[8] = {0, -66, -24, 5, 22, 33, 39, 21};
int Eval::passed_pawn_king_dist_us[8]   = {0, 23, 24, 0, -10, -11, 0, -17};
int Eval::passed_pawn_mg[8]             = {0, -16, -16, -12, 23, 68, 95, 0};
int Eval::passed_pawn_no_attacks[8]     = {0, 3, 3, 13, 24, 48, 85, 0};
int Eval::passed_pawn_no_them[8]        = {0, 2, 6, 22, 36, 64, 129, 0};
int Eval::passed_pawn_no_us[8]          = {0, 0, -1, 3, 17, 32, 121, 0};
int Eval::pawn_behind_eg[2]             = {4, -8};
int Eval::pawn_behind_mg[2]             = {-4, -28};
int Eval::pawn_doubled_eg[2]            = {-20, -15};
int Eval::pawn_doubled_mg[2]            = {-15, 1};
int Eval::pawn_isolated_eg[2]           = {-7, -32};
int Eval::pawn_isolated_mg[2]           = {-13, -28};

int Eval::queen_attack_king = 39;
int Eval::queen_in_danger   = -38;
int Eval::queen_mob_eg[28]  = {-3, -30, -17, -27, -36, -37, -30, -24, -27, -21, -17, -11, -1, 4, 2, 5, 2, 6, 3, 1, -15, -16, -15, -34, -34, -28, -12, -16};
int Eval::queen_mob_mg[28]  = {-19, -34, -15, -15, -13, -10, -10, -11, -6, -1, 2, -1, -3, 2, 3, 5, 7, 3, 9, 13, 25, 34, 33, 58, 44, 41, 14, 19};
int Eval::rook_attack_king  = 11;
int Eval::rook_in_danger    = -34;
int Eval::rook_mob_eg[15]   = {-46, -28, -24, -9, -5, -2, 5, 9, 14, 19, 21, 27, 27, 15, 3};
int Eval::rook_mob_mg[15]   = {-28, -20, -13, -16, -17, -10, -9, -3, -1, 2, 4, 1, 6, 20, 28};
int Eval::rook_open_file    = 13;

int Eval::bishop_pst_eg[64] = {7, 13,  12,  10,  24,  -5,  -19, -14, 2,   9,   3,   16,  -1,  -7,  -5,  -21, -8,  7,   4,   -17, 12,  -5,
                               1, -19, -7,  3,   -6,  5,   7,   6,   -11, -3,  -18, -16, 7,   2,   -7,  -6,  -13, -22, -23, -19, -15, -5,
                               2, -10, -14, -16, -33, -36, -34, -15, -16, -33, -26, -64, -46, -36, -27, -15, -26, -16, -63, -38};
int Eval::bishop_pst_mg[64] = {-65, -58, -27, -41, -46, -70, 15, -35, -40, -26, -43, -25, -39, -10, -46, -59, -5,  -3,  2,   13, 22, 64,
                               29,  4,   -27, -3,  7,   41,  14, 14,  -5,  -24, -3,  -12, 2,   17,  24,  -3,  2,   -8,  -19, 7,  0,  6,
                               2,   5,   4,   9,   7,   0,   12, -13, 1,   16,  25,  16,  -5,  20,  -23, -23, -14, -14, -8,  -23};
int Eval::king_pst_eg[64]   = {-116, -11, 13, -34, -25, 22, 18, -136, -45, 40, 26, 16, 17, 36, 51, -75, 18,  41, 40, 35, 25, 33, 52, -5,  -8,  24,  34,  36, 31,  38, 23,  -4,
                             -25,  5,   17, 24,  32,  25, 7,  -20,  -31, -5, 8,  15, 25, 21, 2,  -19, -22, -9, -2, 5,  12, 13, -5, -27, -44, -35, -16, 2,  -20, -8, -38, -66};
int Eval::king_pst_mg[64]   = {-83, 106, 122, 137, 123, 72,  44, -34, -2,  67,  37,  44, 43,  27, -13, -60, -44, 47,  7,   17, -2,  12,
                             16,  -47, -16, 52,  21,  -12, -1, 5,   30,  -69, -23, 25, 6,   7,  -15, -15, -14, -63, -49, 16, -13, -12,
                             -24, -37, -10, -53, -12, -14, 6,  -40, -36, -32, 13,  0,  -52, 16, 0,   -73, -34, -50, 32,  7};
int Eval::knight_pst_eg[64] = {-33, -12, 0,   -25, -30, -18, -14, -54, -31, -16, -15, -14, -3,   -40, -21, -22, -37, -17, -3,  3,   -12, -13,
                               -22, -28, -32, -11, -1,  15,  16,  5,   -10, -26, -28, -10, 3,    10,  9,   10,  -11, -27, -54, -28, -19, -5,
                               -5,  -24, -37, -29, -60, -37, -40, -34, -37, -34, -29, -48, -159, -75, -46, -30, -46, -52, -67, -37};
int Eval::knight_pst_mg[64] = {-221, -103, -59, -11, 38,  -31, -103, -255, -66, -40, 5,   23,  37,   35,  -11, -32, -22, -3,  42,  39,  75, 115,
                               10,   -33,  -23, 6,   39,  66,  19,   61,   11,  25,  -25, -13, 15,   19,  32,  11,  26,  -6,  -39, -27, -8, 4,
                               20,   1,    -4,  -40, -46, -54, -31,  -8,   -14, -20, -39, -13, -104, -44, -54, -53, -22, -24, -59, -2};
int Eval::pawn_pst_eg[64]   = {0,  0,  0,  0, 0, 0, 0, 0, 0,  24, 49, 29, 34, 54, 74, 26, 33, 39, 28, 15, 21, 13, 20, 6,  31, 25, 19, 1, 2, -2, 9, 13,
                             18, 18, 12, 5, 4, 8, 5, 2, 13, 9,  10, 9,  12, 7,  -4, -4, 17, 9,  15, 12, 26, 11, -6, -2, 0,  0,  0,  0, 0, 0,  0, 0};
int Eval::pawn_pst_mg[64]   = {0, 0, 0, 0,  0,  0, 0, 0,   206, 124, 96, 113, 98, 36, 9,  13, 19, 25,  43,  36,  58,  77, 64, 30,  10, 16, 9, 25, 23, 12, -4, -7,
                             0, 4, 9, 10, 11, 6, 0, -11, 0,   -1,  -3, 8,   9,  6,  18, -4, -7, -11, -14, -11, -14, 23, 29, -16, 0,  0,  0, 0,  0,  0,  0,  0};
int Eval::queen_pst_eg[64]  = {5,   14,  10,  23,  20,  2,   9,   8,   5,   40,  40,   38,  49,  25,  25,  10,  -22, 7,   38,   37,  40,  20,
                              2,   12,  -10, -5,  24,  45,  46,  32,  38,  5,   -32,  -5,  -2,  23,  17,  4,   -3,  3,   -50,  -42, -20, -36,
                              -18, -24, -25, -39, -73, -47, -64, -61, -57, -97, -108, -64, -89, -85, -67, -61, -84, -96, -123, -110};
int Eval::queen_pst_mg[64]  = {-10, 4,   10,  13,  14,  86,  64, 57,  -36, -60, -31, -10, -10, 43, 5, 39, -9,  -17, -14, -4, 36, 63, 69, 16,  -34, -28, -30, -20, -16, -6,  7,   -8,
                              -22, -17, -15, -16, -10, -13, 4,  -13, -21, -4,  -10, -13, -4,  7,  6, -9, -14, -3,  2,   4,  3,  26, 16, -12, 8,   -8,  -5,  4,   5,   -18, -58, -9};
int Eval::rook_pst_eg[64]   = {46, 47, 33, 28, 36, 42, 37, 29, 44, 41, 28, 23, 20,  22, 20, 16, 49,  27, 28,  10,  12,  21,  13,  32, 40,  35,  23,  14,  13,  21,  20,  18,
                             28, 34, 23, 11, 10, 17, 11, 19, 16, 14, 1,  -6, -11, -1, -1, 3,  -12, -8, -12, -24, -23, -19, -21, -6, -13, -15, -12, -20, -26, -16, -11, -19};
int Eval::rook_pst_mg[64]   = {1,   3,   25,  31,  24,  24,  35, 54,  3,   -8,  28,  40,  39,  56,  45, 39,  -18, 8,   14,  31, 59, 67,  51, 11,  -41, -23, 4, 20, 16, 17, 24, -11,
                             -46, -43, -26, -16, -22, -23, -7, -45, -48, -35, -33, -31, -10, -21, -6, -25, -45, -33, -18, -4, -3, -10, -7, -68, -16, -9,  0, 3,  7,  4,  8,  -18};
