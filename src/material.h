#pragma once

#include <array>
#include <algorithm>
#include "board.h"
#include "piece.h"

constexpr int RECOGNIZEDDRAW = 1;

class Material {
public:
  Material() {
    max_value_without_pawns = 2 * (2 * piece_value[Knight] + 2 * piece_value[Bishop] + 2 * piece_value[Rook] + piece_value[Queen]);

    max_value = max_value_without_pawns + 2 * 8 * piece_value[Pawn];
  }

  void clear() {
    key[0] = key[1]   = 0;
    material_value[0] = material_value[1] = 0;
  }

  void remove(const int p) {
    update_key(p >> 3, p & 7, -1);
    material_value[p >> 3] -= piece_value[p & 7];
  }

  void add(const int p) {
    update_key(p >> 3, p & 7, 1);
    material_value[p >> 3] += piece_value[p & 7];
  }

  void update_key(const int c, const int p, const int delta) {
    if (p == King)
      return;
    const int x = count(c, p) + delta;
    key[c] &= ~(15 << piece_bit_shift[p]);
    key[c] |= x << piece_bit_shift[p];
  }

  int count(const int c, const int p) { return key[c] >> piece_bit_shift[p] & 15; }

  void make_move(const uint32_t m) {
    if (is_capture(m))
      remove(moveCaptured(m));

    if (move_type(m) & PROMOTION)
    {
      remove(move_piece(m));
      add(move_promoted(m));
    }
  }

  bool is_kx(const int c) { return key[c] == (key[c] & 15); }

  bool is_kx() { return is_kx(0) && is_kx(1); }

  int value() { return material_value[0] + material_value[1]; }

  int value(const int c) { return material_value[c]; }

  int balance() { return material_value[0] - material_value[1]; }

  int pawn_value() { return (key[0] & 15) * piece_value[Pawn] + (key[1] & 15) * piece_value[Pawn]; }

  int pawn_count() { return (key[0] & 15) + (key[1] & 15); }

  int pawn_count(int side) { return key[side] & 15; }

  int evaluate(int &flags, int eval, int side_to_move, const Board *board) {
    this->flags = 0;
    uint32_t key1;
    uint32_t key2;
    int score;
    int side1;

    if (key[side_to_move] >= key[side_to_move ^ 1])
    {
      key1  = key[side_to_move];
      key2  = key[side_to_move ^ 1];
      side1 = side_to_move;
      score = eval;
    } else
    {
      key1  = key[side_to_move ^ 1];
      key2  = key[side_to_move];
      side1 = side_to_move ^ 1;
      score = -eval;
    }
    const int side2 = side1 ^ 1;
    const int pc1   = pawn_count(side1);
    const int pc2   = pawn_count(side2);

    this->board = board;
    drawish     = 0;

    switch (key1 & ~all_pawns)
    {
    case kqb:
      score = KQBKX(score, key2);
      break;

    case kqn:
      score = KQNKX(score, key2);
      break;

    case krb:
      score = KRBKX(score, key2);
      break;

    case krn:
      score = KRNKX(score, key2);
      break;

    case kr:
      score = KRKX(score, key2);
      break;

    case kbb:
      score = KBBKX(score, key2);
      break;

    case kbn:
      score = KBNKX(score, key2, pc1, pc2, side1);
      break;

    case kb:
      score = KBKX(score, key1, key2, pc1, pc2, side1, side2, side_to_move);
      break;

    case kn:
      score = KNKX(score, key2, pc1, pc2, side1, side2, side_to_move);
      break;

    case knn:
      score = KNNKX(score, key2, pc1);
      break;

    case k:
      score = KKx(score, key1, key2, pc1, pc2, side1);
      break;

    default:
      break;
    }

    if (drawish)
    {
      const int drawish_score = score / drawish;

      if (pc1 + pc2 == 0)
      {
        score = drawish_score;
      } else if (pc1 == 0)
      {
        score = std::min(drawish_score, score);
      } else if (pc2 == 0)
      { score = std::max(drawish_score, score); }
    }
    flags = this->flags;
    return side1 != side_to_move ? -score : score;
  }

  int KQBKX(const int eval, const uint32_t key2) {
    switch (key2 & ~all_pawns)
    {
    case kq:
      drawish = 16;
      break;

    default:
      break;
    }
    return eval;
  }

  int KQNKX(const int eval, const uint32_t key2) {
    switch (key2 & ~all_pawns)
    {
    case kq:
      drawish = 16;
      break;

    default:
      break;
    }
    return eval;
  }

  int KRBKX(int eval, uint32_t key2) {
    switch (key2 & ~all_pawns)
    {
    case kr:
      drawish = 16;
      break;

    case kbb:
    case kbn:
    case knn:
      drawish = 8;
      break;

    default:
      break;
    }
    return eval;
  }

  int KRNKX(int eval, uint32_t key2) {
    switch (key2 & ~all_pawns)
    {
    case kr:
      drawish = 32;
      break;

    case kbb:
    case kbn:
    case knn:
      drawish = 16;
      break;

    default:
      break;
    }
    return eval;
  }

  int KRKX(int eval, uint32_t key2) {
    switch (key2 & ~all_pawns)
    {
    case kbb:
    case kbn:
    case knn:
      drawish = 16;
      break;

    case kb:
    case kn:
      drawish = 8;
      break;

    default:
      break;
    }
    return eval;
  }

  int KBBKX(int eval, uint32_t key2) {
    switch (key2 & ~all_pawns)
    {
    case kb:
      drawish = 16;
      break;

    case kn:
      break;

    default:
      break;
    }
    return eval;
  }

  int KBNKX(int eval, uint32_t key2, int pc1, int pc2, int side1) {
    switch (key2 & ~all_pawns)
    {
    case k:
      if (pc1 + pc2 == 0)
      {
        return KBNK(eval, side1);
      }
      break;

    case kb:
      drawish = 8;
      break;

    case kn:
      drawish = 4;
      break;

    default:
      break;
    }
    return eval;
  }

  int KBNK(int eval, int side1) {
    int loosing_kingsq           = board->king_square[side1 ^ 1];
    int a_winning_cornersq       = a8;
    int another_winning_cornersq = h1;

    if (is_dark(lsb(board->bishops(side1))))
    {
      a_winning_cornersq       = a1;
      another_winning_cornersq = h8;
    }
    return eval + 175 - std::min(25 * dist[a_winning_cornersq][loosing_kingsq], 25 * dist[another_winning_cornersq][loosing_kingsq]);
  }

  int KBKX(const int eval, const uint32_t key1, const uint32_t key2, const int pc1, const int pc2, const int side1, const int side2, const int side_to_move) {
    if (pc1 > 0)
      return KBxKX(eval, key1, key2, side1);

    switch (key2 & ~all_pawns)
    {
    case k: {
      if (pc1 + pc2 == 0)
      {
        return draw_score();
      } else if (pc1 == 0 && pc2 == 1)
      {
        const uint64_t &bishopbb = board->bishops(side1);

        if (side1 == side_to_move || !board->is_attacked(lsb(board->bishops(side1)), side2))
        {
          if (pawn_front_span[side2][lsb(board->pawns(side2))] & (bishopAttacks(lsb(bishopbb), board->occupied) | bishopbb))
          {
            return draw_score();
          }
        }
      }
      break;
    }

    case kb:
    case knn:
    case kn:
      drawish = 16;
      break;

    default:
      break;
    }
    return std::min(0, eval);
  }

  int KNKX(const int eval, const uint32_t key2, const int pc1, const int pc2, const int side1, const int side2, const int side_to_move) {
    switch (key2 & ~all_pawns)
    {
    case k: {
      if (pc1 + pc2 == 0)
        return draw_score();

      if (pc1 == 0 && pc2 == 1)
      {
        const uint64_t &knightbb = board->knights(side1);

        if (side1 == side_to_move || !board->is_attacked(lsb(board->knights(side1)), side2))
        {
          if (pawn_front_span[side2][lsb(board->pawns(side2))] & (knightAttacks(lsb(knightbb)) | knightbb))
            return draw_score();
        }
      }
      break;
    }

    case kn:
      drawish = 16;
      break;

    default:
      break;
    }
    return pc1 == 0 ? std::min(0, eval) : eval;
  }

  int KNNKX(const int eval, const uint32_t key2, const int pc1) {
    switch (key2 & ~all_pawns)
    {
    case k:
    case kn:
      drawish = 32;
      break;

    default:
      break;
    }
    return pc1 == 0 ? std::min(0, eval) : eval;
  }

  int KKx(const int eval, const uint32_t key1, const uint32_t key2, const int pc1, const int pc2, const int side1) {
    if (pc1 + pc2 == 0)
    {
      return draw_score();
    }
    if (pc2 > 0)
    { return KxKx(eval, key1, key2, pc1, pc2, side1); }
    return eval;
  }

  int KBxKX(const int eval, const uint32_t key1, const uint32_t key2, const int side1) {
    switch (key2 & ~all_pawns)
    {
    case kb:
      if (!same_color(lsb(board->bishops(0)), lsb(board->bishops(1))) && abs(pawn_count(0) - pawn_count(1)) <= 2)
      {
        return eval / 2;
      }
      break;

    case k:
      return KBxKx(eval, key1, key2, side1);

    default:
      break;
    }
    return eval;
  }

  int KBxKx(const int eval, const uint32_t key1, const uint32_t key2, const int side1) {
    if ((key1 & all_pawns) == 1 && (key2 & all_pawns) == 0)
    {
      return KBpK(eval, side1);
    }
    return eval;
  }

  // fen 8/6k1/8/8/3K4/5B1P/8/8 w - - 0 1
  int KBpK(const int eval, const int side1) {
    const auto pawnsq1    = lsb(board->pawns(side1));
    const auto promosq1 = static_cast<Square>(side1 == 1 ? file_of(pawnsq1) : file_of(pawnsq1) + 56);

    if (!same_color(promosq1, lsb(board->bishops(side1))))
    {
      if (const auto &bbk2 = board->king(side1 ^ 1); promosq1 == h8 && bbk2 & corner_h8 || promosq1 == a8 && bbk2 & corner_a8 || promosq1 == h1 && bbk2 & corner_h1 || promosq1 == a1 && bbk2 & corner_a1)
      {
        return draw_score();
      }
    }
    return eval;
  }

  int KxKx(const int eval, uint32_t key1, uint32_t key2, const int pc1, const int pc2, const int side1) {
    return pc1 == 1 && pc2 == 0 ? KpK(eval, side1) : eval;
  }

  int KpK(const int eval, const int side1) {
    const auto pawnsq1  = lsb(board->pawns(side1));
    const auto promosq1 = static_cast<Square>(side1 == 1 ? file_of(pawnsq1) : file_of(pawnsq1) + 56);
    
    if (const auto &bbk2 = board->king(side1 ^ 1); promosq1 == h8 && bbk2 & corner_h8 || promosq1 == a8 && bbk2 & corner_a8 || promosq1 == h1 && bbk2 & corner_h1 || promosq1 == a1 && bbk2 & corner_a1)
      return draw_score();
    return eval;
  }

  int draw_score() {
    flags |= RECOGNIZEDDRAW;
    return 0;
  }

  int drawish{};
  int flags{};
  std::array<uint32_t, 2> key{};
  std::array<int, 2> material_value{};
  const Board *board{};
  int max_value_without_pawns{};
  int max_value{};

  static constexpr std::array<int, 7> piece_bit_shift{0, 4, 8, 12, 16, 20};
  static constexpr std::array<int, 6> piece_value{100, 400, 400, 600, 1200, 0};

  static constexpr uint32_t k   = 0x00000;
  static constexpr uint32_t kp  = 0x00001;
  static constexpr uint32_t kn  = 0x00010;
  static constexpr uint32_t kb  = 0x00100;
  static constexpr uint32_t kr  = 0x01000;
  static constexpr uint32_t kq  = 0x10000;
  static constexpr uint32_t krr = 0x02000;
  static constexpr uint32_t kbb = 0x00200;
  static constexpr uint32_t kbn = 0x00110;
  static constexpr uint32_t knn = 0x00020;
  static constexpr uint32_t krn = 0x01010;
  static constexpr uint32_t krb = 0x01100;
  static constexpr uint32_t kqb = 0x10100;
  static constexpr uint32_t kqn = 0x10010;

  static constexpr uint32_t all_pawns = 0xf;
};


#define piece_value(p) (Material::piece_value[p & 7])