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

}// namespace

void Material::clear() {
  key.fill(0);
  material_value.fill(0);
}

void Material::remove(const Piece pc) {
  const auto c  = color_of(pc);
  const auto pt = type_of(pc);
  update_key(c, pt, -1);
  material_value[c] -= piece_values[pt];
}

void Material::add(const Piece pc) {
  const auto c  = color_of(pc);
  const auto pt = type_of(pc);
  update_key(c, pt, 1);
  material_value[c] += piece_values[pt];
}

void Material::update_key(const Color c, const PieceType pt, const int delta) {
  if (pt == KING)
    return;
  const auto x = count(c, pt) + delta;
  key[c] &= ~(15 << piece_bit_shift[pt]);
  key[c] |= x << piece_bit_shift[pt];
}

int Material::count(const Color c, const PieceType pt) {
  return key[c] >> piece_bit_shift[pt] & 15;
}

void Material::make_move(const Move m) {
  if (is_capture(m))
    remove(move_captured(m));

  if (is_promotion(m))
  {
    remove(move_piece(m));
    add(move_promoted(m));
  }
}

int Material::pawn_value() {
  return static_cast<int>(key[WHITE] & all_pawns) * piece_values[PAWN] + static_cast<int>(key[BLACK] & all_pawns) * piece_values[PAWN];
}

template<Color Us>
int Material::evaluate(int &flags, const int eval, const Board *b) {
  constexpr auto Them = ~Us;
  this->board              = b;
  drawish = material_flags = 0;

  uint32_t strong_key;
  uint32_t weak_key;
  int score;
  Color strong_side;

  if (key[Us] >= key[Them])
  {
    strong_key  = key[Us];
    weak_key    = key[Them];
    strong_side = Us;
    score       = eval;
  } else
  {
    strong_key  = key[Them];
    weak_key    = key[Us];
    strong_side = Them;
    score       = -eval;
  }

  const auto weak_side         = ~strong_side;
  const auto strong_pawn_count = pawn_count(strong_side);
  const auto weak_pawn_count   = pawn_count(weak_side);

  switch (strong_key & ~all_pawns)
  {
  case kqb:
    score = KQBKX(score, weak_key);
    break;

  case kqn:
    score = KQNKX(score, weak_key);
    break;

  case krb:
    score = KRBKX(score, weak_key);
    break;

  case krn:
    score = KRNKX(score, weak_key);
    break;

  case kr:
    score = KRKX(score, weak_key);
    break;

  case kbb:
    score = KBBKX(score, weak_key);
    break;

  case kbn:
    score = KBNKX(score, weak_key, strong_pawn_count, weak_pawn_count, strong_side);
    break;

  case kb:
    score = KBKX(score, strong_key, weak_key, strong_pawn_count, weak_pawn_count, strong_side, weak_side, Us);
    break;

  case kn:
    score = KNKX(score, weak_key, strong_pawn_count, weak_pawn_count, strong_side, weak_side, Us);
    break;

  case knn:
    score = KNNKX(score, weak_key, strong_pawn_count);
    break;

  case k:
    score = KKx(score, strong_pawn_count, weak_pawn_count, strong_side);
    break;

  default:
    break;
  }

  if (drawish)
  {
    if (const auto drawish_score = score / drawish; strong_pawn_count + weak_pawn_count == 0)
      score = drawish_score;
    else if (strong_pawn_count == 0)
      score = std::min<int>(drawish_score, score);
    else if (weak_pawn_count == 0)
      score = std::max<int>(drawish_score, score);
  }

  flags = material_flags;
  return strong_side != Us ? -score : score;
}

template int Material::evaluate<WHITE>(int &, int, const Board *);
template int Material::evaluate<BLACK>(int &, int, const Board *);

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

  default:
    break;
  }
  return eval;
}

int Material::KBNKX(const int eval, const uint32_t key2, const int pc1, const int pc2, const Color c1) {
  switch (key2 & ~all_pawns)
  {
  case k:
    if (pc1 + pc2 == 0)
      return KBNK(eval, c1);
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

int Material::KBNK(const int eval, const Color c1) const {
  const auto loosing_kingsq = board->king_sq(~c1);

  constexpr auto get_winning_squares = [](const bool dark) { return dark ? std::make_pair(A1, H8) : std::make_pair(A8, H1); };

  const auto dark = is_dark(lsb(board->pieces(BISHOP, c1)));

  const auto [first_corner, second_corner] = get_winning_squares(dark);

  return eval + 175 - (25 * std::min<int>(distance(first_corner, loosing_kingsq), distance(second_corner, loosing_kingsq)));
}

int Material::KBKX(const int eval, const uint32_t key1, const uint32_t key2, const int pc1, const int pc2, const Color c1, const Color c2, const Color c) {
  if (pc1 > 0)
    return KBxKX(eval, key1, key2, c1);

  switch (key2 & ~all_pawns)
  {
  case k: {
    if (pc1 + pc2 == 0)
      return draw_score();
    if (pc1 == 0 && pc2 == 1)
    {
      if (c1 == c || !board->is_attacked(lsb(board->pieces(BISHOP, c1)), c2))
      {
        if (const auto bishopbb = board->pieces(BISHOP, c1); pawn_front_span[c2][lsb(board->pieces(PAWN, c2))] & (piece_attacks_bb<BISHOP>(lsb(bishopbb), board->pieces()) | bishopbb))
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

int Material::KNKX(const int eval, const uint32_t key2, const int pc1, const int pc2, const Color c1, const Color c2, const Color c) {
  switch (key2 & ~all_pawns)
  {
  case k: {
    if (pc1 + pc2 == 0)
      return draw_score();

    if (pc1 == 0 && pc2 == 1)
    {
      if (c1 == c || !board->is_attacked(lsb(board->pieces(KNIGHT, c1)), c2))
      {
        if (const auto knightbb = board->pieces(KNIGHT, c1); pawn_front_span[c2][lsb(board->pieces(PAWN, c2))] & (piece_attacks_bb<KNIGHT>(lsb(knightbb)) | knightbb))
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

int Material::KKx(const int eval, const int pc1, const int pc2, const Color c1) {
  return pc1 + pc2 == 0
                    ? draw_score()
                    : pc2 > 0
                      ? KxKx(eval, pc1, pc2, c1)
                      : eval;
}

int Material::KBxKX(const int eval, const uint32_t key1, const uint32_t key2, const Color c1) {
  switch (key2 & ~all_pawns)
  {
  case kb:
    if (!same_color(lsb(board->pieces(BISHOP, WHITE)), lsb(board->pieces(BISHOP, BLACK))) && util::abs(pawn_count(WHITE) - pawn_count(BLACK)) <= 2)
      return eval / 2;

    break;

  case k:
    return KBxKx(eval, key1, key2, c1);

  default:
    break;
  }
  return eval;
}

int Material::KBxKx(const int eval, const uint32_t key1, const uint32_t key2, const Color c1) {
  return (key1 & all_pawns) == 1 && (key2 & all_pawns) == 0 ? KBpK(eval, c1) : eval;
}

int Material::KBpK(const int eval, const Color c1) {
  const auto pawnsq1  = lsb(board->pieces(PAWN, c1));
  const auto promosq1 = static_cast<Square>(c1 == BLACK ? file_of(pawnsq1) : file_of(pawnsq1) + 56);

  if (!same_color(promosq1, lsb(board->pieces(BISHOP, c1))))
  {
    if (const auto bbk2 = board->king(~c1);
        (promosq1 == H8 && bbk2 & corner_h8) || (promosq1 == A8 && bbk2 & corner_a8) || (promosq1 == H1 && bbk2 & corner_h1) || (promosq1 == A1 && bbk2 & corner_a1))
      return draw_score();
  }
  return eval;
}

int Material::KxKx(const int eval, const int pc1, const int pc2, const Color c1) {
  return pc1 == 1 && pc2 == 0 ? KpK(eval, c1) : eval;
}

int Material::KpK(const int eval, const Color c1) {
  const auto pawnsq1  = lsb(board->pieces(PAWN, c1));
  const auto promosq1 = static_cast<Square>(c1 == BLACK ? file_of(pawnsq1) : file_of(pawnsq1) + 56);
  const auto bbk2     = board->king(~c1);

  return (promosq1 == H8 && bbk2 & corner_h8) || (promosq1 == A8 && bbk2 & corner_a8) || (promosq1 == H1 && bbk2 & corner_h1) || (promosq1 == A1 && bbk2 & corner_a1)
       ? draw_score()
       : eval;
}

int Material::draw_score() {
  material_flags |= RECOGNIZEDDRAW;
  return 0;
}
