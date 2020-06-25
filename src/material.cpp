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

#include <array>
#include <algorithm>

#include "material.h"
#include "board.h"
#include "magic.h"
#include "types.h"

namespace {

constexpr std::array<int, 7> piece_bit_shift{0, 4, 8, 12, 16, 20};

constexpr uint32_t k   = 0x00000;
constexpr uint32_t kp  = 0x00001;
constexpr uint32_t kn  = 0x00010;
constexpr uint32_t kb  = 0x00100;
constexpr uint32_t kr  = 0x01000;
constexpr uint32_t kq  = 0x10000;
constexpr uint32_t krr = 0x02000;
constexpr uint32_t kbb = 0x00200;
constexpr uint32_t kbn = 0x00110;
constexpr uint32_t knn = 0x00020;
constexpr uint32_t krn = 0x01010;
constexpr uint32_t krb = 0x01100;
constexpr uint32_t kqb = 0x10100;
constexpr uint32_t kqn = 0x10010;

constexpr uint32_t all_pawns = 0xf;

}

void Material::clear() {
  key.fill(0);
  material_value.fill(0);
}

void Material::remove(const int p) {
  const auto c = static_cast<Color>(p >> 3);
  update_key(c, p & 7, -1);
  material_value[p >> 3] -= piece_values[p & 7];
}

void Material::add(const int p) {
  const auto c = static_cast<Color>(p >> 3);
  update_key(c, p & 7, 1);
  material_value[p >> 3] += piece_values[p & 7];
}

void Material::update_key(const Color c, const int p, const int delta) {
  if (p == King)
    return;
  const auto x = count(c, p) + delta;
  key[c] &= ~(15 << piece_bit_shift[p]);
  key[c] |= x << piece_bit_shift[p];
}

int Material::count(const Color c, const int p) { return key[c] >> piece_bit_shift[p] & 15; }

void Material::make_move(const Move m) {
  if (is_capture(m))
    remove(move_captured(m));

  if (is_promotion(m))
  {
    remove(move_piece(m));
    add(move_promoted(m));
  }
}

int Material::pawn_value() { return static_cast<int>(key[WHITE] & 15) * piece_values[Pawn] + static_cast<int>(key[BLACK] & 15) * piece_values[Pawn]; }

int Material::evaluate(int &flags, const int eval, const Color side_to_move, const Board *b) {
  material_flags = 0;
  uint32_t key1;
  uint32_t key2;
  int score;
  Color side1;

  if (key[side_to_move] >= key[~side_to_move])
  {
    key1  = key[side_to_move];
    key2  = key[~side_to_move];
    side1 = side_to_move;
    score = eval;
  } else
  {
    key1  = key[~side_to_move];
    key2  = key[side_to_move];
    side1 = ~side_to_move;
    score = -eval;
  }
  const auto side2 = ~side1;
  const auto pc1     = pawn_count(side1);
  const auto pc2     = pawn_count(side2);

  this->board = b;
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
    if (const auto drawish_score = score / drawish; pc1 + pc2 == 0)
      score = drawish_score;
    else if (pc1 == 0)
      score = std::min(drawish_score, score);
    else if (pc2 == 0)
      score = std::max(drawish_score, score);
  }

  flags = material_flags;
  return side1 != side_to_move ? -score : score;
}

int Material::KQBKX(const int eval, const uint32_t key2) {
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

int Material::KQNKX(const int eval, const uint32_t key2) {
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

int Material::KRBKX(const int eval, const uint32_t key2) {
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

int Material::KRNKX(const int eval, const uint32_t key2) {
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

int Material::KRKX(const int eval, const uint32_t key2) {
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

int Material::KBBKX(const int eval, const uint32_t key2) {
  switch (key2 & ~all_pawns)
  {
  case kb:
    drawish = 16;
    break;

    //case kn:
    //  break;

  default:
    break;
  }
  return eval;
}

int Material::KBNKX(const int eval, const uint32_t key2, const int pc1, const int pc2, const Color side1) {
  switch (key2 & ~all_pawns)
  {
  case k:
    if (pc1 + pc2 == 0)
      return KBNK(eval, side1);
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

int Material::KBNK(const int eval, const Color side1) const {
  const auto loosing_kingsq = board->king_sq(~side1);

  constexpr auto get_winning_squares = [](const bool dark)
  {
    return dark ? std::make_pair(a1, h8) : std::make_pair(a8, h1);
  };

  const auto dark = is_dark(lsb(board->pieces(Bishop, side1)));

  const auto [first_corner, second_corner] = get_winning_squares(dark);

  return eval + 175 - std::min(25 * distance(first_corner, loosing_kingsq), 25 * distance(second_corner, loosing_kingsq));
}

int Material::KBKX(const int eval, const uint32_t key1, const uint32_t key2, const int pc1, const int pc2, const Color side1, const Color side2, const Color side_to_move) {
  if (pc1 > 0)
    return KBxKX(eval, key1, key2, side1);

  switch (key2 & ~all_pawns)
  {
  case k: {
    if (pc1 + pc2 == 0)
      return draw_score();
    if (pc1 == 0 && pc2 == 1)
    {
      if (side1 == side_to_move || !board->is_attacked(lsb(board->pieces(Bishop, side1)), side2))
      {
        if (const auto bishopbb = board->pieces(Bishop, side1); pawn_front_span[side2][lsb(board->pieces(Pawn, side2))] & (piece_attacks_bb<Bishop>(lsb(bishopbb), board->pieces()) | bishopbb))
          return draw_score();
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

int Material::KNKX(const int eval, const uint32_t key2, const int pc1, const int pc2, const Color side1, const Color side2, const Color side_to_move) {
  switch (key2 & ~all_pawns)
  {
  case k: {
    if (pc1 + pc2 == 0)
      return draw_score();

    if (pc1 == 0 && pc2 == 1)
    {
      if (side1 == side_to_move || !board->is_attacked(lsb(board->pieces(Knight, side1)), side2))
      {
        if (const auto knightbb = board->pieces(Knight, side1); pawn_front_span[side2][lsb(board->pieces(Pawn, side2))] & (piece_attacks_bb<Knight>(lsb(knightbb)) | knightbb))
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

int Material::KNNKX(const int eval, const uint32_t key2, const int pc1) {
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

int Material::KKx(const int eval, const uint32_t key1, const uint32_t key2, const int pc1, const int pc2, const Color side1) {
  if (pc1 + pc2 == 0)
    return draw_score();
  if (pc2 > 0)
    return KxKx(eval, key1, key2, pc1, pc2, side1);
  return eval;
}

int Material::KBxKX(const int eval, const uint32_t key1, const uint32_t key2, const Color side1) {
  switch (key2 & ~all_pawns)
  {
  case kb:
    if (!same_color(lsb(board->pieces(Bishop, WHITE)), lsb(board->pieces(Bishop, BLACK))) && abs(pawn_count(WHITE) - pawn_count(BLACK)) <= 2)
      return eval / 2;

    break;

  case k:
    return KBxKx(eval, key1, key2, side1);

  default:
    break;
  }
  return eval;
}

int Material::KBxKx(const int eval, const uint32_t key1, const uint32_t key2, const Color side1) {
  if ((key1 & all_pawns) == 1 && (key2 & all_pawns) == 0)
    return KBpK(eval, side1);

  return eval;
}

int Material::KBpK(const int eval, const Color side1) {
  const auto pawnsq1  = lsb(board->pieces(Pawn, side1));
  const auto promosq1 = static_cast<Square>(side1 == BLACK ? file_of(pawnsq1) : file_of(pawnsq1) + 56);

  if (!same_color(promosq1, lsb(board->pieces(Bishop, side1))))
  {
    if (const auto bbk2 = board->king(~side1); (promosq1 == h8 && bbk2 & corner_h8) || (promosq1 == a8 && bbk2 & corner_a8) || (promosq1 == h1 && bbk2 & corner_h1) || (
                                                  promosq1 == a1 && bbk2 & corner_a1))
      return draw_score();
  }
  return eval;
}

int Material::KxKx(const int eval, [[maybe_unused]] const uint32_t key1, [[maybe_unused]] const uint32_t key2, const int pc1, const int pc2, const Color side1) {
  return pc1 == 1 && pc2 == 0 ? KpK(eval, side1) : eval;
}

int Material::KpK(const int eval, const Color side1) {
  const auto pawnsq1  = lsb(board->pieces(Pawn, side1));
  const auto promosq1 = static_cast<Square>(side1 == BLACK ? file_of(pawnsq1) : file_of(pawnsq1) + 56);

  if (const auto bbk2 = board->king(~side1); (promosq1 == h8 && bbk2 & corner_h8)
                                          || (promosq1 == a8 && bbk2 & corner_a8)
                                          || (promosq1 == h1 && bbk2 & corner_h1)
                                          || (promosq1 == a1 && bbk2 & corner_a1))
    return draw_score();
  return eval;
}

int Material::draw_score() {
  material_flags |= RECOGNIZEDDRAW;
  return 0;
}
