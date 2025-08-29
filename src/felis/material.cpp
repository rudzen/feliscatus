// Copyright (C) 2008-2016 Gunnar Harms (Bobcat author)
// Copyright (C) 2017      FireFather (Tomcat author)
// Copyright (C) 2020-2025 Rudy Alex Kohn
// See end of file for extended copyright information.

#include <array>
#include <algorithm>
#include <felis/material.hpp>
#include <felis/board.hpp>

namespace
{

constexpr int RECOGNIZEDDRAW = 1;
constexpr std::array<int, 7> piece_bit_shift{0, 4, 8, 12, 16, 20};

[[nodiscard]]
i32 draw_score(Material *m)
{
  m->material_flags |= RECOGNIZEDDRAW;
  return 0;
}

void update_key(Material *m, const Color c, const PieceType pt, const int delta)
{
  if (pt == KING)
    return;
  const auto x = material::count(m, c, pt) + delta;
  m->key[c] &= ~(15 << piece_bit_shift[pt]);
  m->key[c] |= x << piece_bit_shift[pt];
}

i32 pawn_count(const Material *m, const Color c)
{
  return static_cast<int>(m->key[c] & 15);
}

[[nodiscard]]
i32 KQBKX(Material *m, const i32 eval, const u32 key2)
{
  switch (key2 & ~all_pawns)
  {
  case kq:
    m->drawish = 16;
    break;

  default:
    break;
  }
  return eval;
}

[[nodiscard]]
i32 KQNKX(Material *m, const i32 eval, const u32 key2)
{
  switch (key2 & ~all_pawns)
  {
  case kq:
    m->drawish = 16;
    break;

  default:
    break;
  }
  return eval;
}

[[nodiscard]]
i32 KRBKX(Material *m, const i32 eval, const u32 key2)
{
  switch (key2 & ~all_pawns)
  {
  case kr:
    m->drawish = 16;
    break;

  case kbb:
  case kbn:
  case knn:
    m->drawish = 8;
    break;

  default:
    break;
  }
  return eval;
}

[[nodiscard]]
i32 KRNKX(Material *m, const i32 eval, const u32 key2)
{
  switch (key2 & ~all_pawns)
  {
  case kr:
    m->drawish = 32;
    break;

  case kbb:
  case kbn:
  case knn:
    m->drawish = 16;
    break;

  default:
    break;
  }
  return eval;
}

[[nodiscard]]
i32 KRKX(Material *m, const i32 eval, const u32 key2)
{
  switch (key2 & ~all_pawns)
  {
  case kbb:
  case kbn:
  case knn:
    m->drawish = 16;
    break;

  case kb:
  case kn:
    m->drawish = 8;
    break;

  default:
    break;
  }
  return eval;
}

[[nodiscard]]
i32 KBBKX(Material *m, const i32 eval, const u32 key2)
{
  switch (key2 & ~all_pawns)
  {
  case kb:
    m->drawish = 16;
    break;

  default:
    break;
  }
  return eval;
}

[[nodiscard]]
i32 KBNK(const Material *m, const i32 eval, const Color c1)
{
  const Square loosing_kingsq = m->board->square<KING>(~c1);

  constexpr auto get_winning_squares = [](const bool dark) {
    return dark ? std::make_pair(A1, H8) : std::make_pair(A8, H1);
  };

  const bool dark = is_dark(lsb(m->board->pieces(BISHOP, c1)));

  const auto [first_corner, second_corner] = get_winning_squares(dark);

  return eval + 175 - (25 * std::min<i32>(distance(first_corner, loosing_kingsq), distance(second_corner, loosing_kingsq)));
}

[[nodiscard]]
i32 KBNKX(Material *m, const i32 eval, const u32 key2, const i32 pc1, const i32 pc2, const Color c1)
{
  switch (key2 & ~all_pawns)
  {
  case k:
    if (pc1 + pc2 == 0)
      return KBNK(m, eval, c1);
    break;

  case kb:
    m->drawish = 8;
    break;

  case kn:
    m->drawish = 4;
    break;

  default:
    break;
  }
  return eval;
}

[[nodiscard]]
i32 KNKX(Material *m, const i32 eval, const u32 key2, const i32 pc1, const i32 pc2, const Color c1, const Color c2, const Color c)
{
  switch (key2 & ~all_pawns)
  {
  case k: {
    if (pc1 + pc2 == 0)
      return draw_score(m);

    if (pc1 == 0 && pc2 == 1)
    {
      if (c1 == c || !m->board->is_attacked(lsb(m->board->pieces(KNIGHT, c1)), c2))
      {
        const Bitboard knightbb = m->board->pieces(KNIGHT, c1);
        if (pawn_front_spanBB(c2, lsb(m->board->pieces(PAWN, c2))) & (piece_attacks_bb<KNIGHT>(lsb(knightbb)) | knightbb))
          return draw_score(m);
      }
    }
    break;
  }

  case kn:
    m->drawish = 16;
    break;

  default:
    break;
  }
  return pc1 == 0 ? std::min<i32>(0, eval) : eval;
}

[[nodiscard]]
i32 KNNKX(Material *m, const i32 eval, const u32 key2, const i32 pc1)
{
  switch (key2 & ~all_pawns)
  {
  case k:
  case kn:
    m->drawish = 32;
    break;

  default:
    break;
  }

  return pc1 == 0 ? std::min<int>(0, eval) : eval;
}

// fen 8/6k1/8/8/3K4/5B1P/8/8 w - - 0 1
[[nodiscard]]
i32 KBpK(Material *m, const i32 eval, const Color c1)
{
  const Square pawnsq1  = lsb(m->board->pieces(PAWN, c1));
  const Square promosq1 = static_cast<Square>(c1 == BLACK ? file_of(pawnsq1) : file_of(pawnsq1) + 56);

  if (!same_color(promosq1, lsb(m->board->pieces(BISHOP, c1))))
  {
    const Bitboard bbk2 = m->board->pieces(KING, ~c1);
    if ((promosq1 == H8 && bbk2 & corner_h8) || (promosq1 == A8 && bbk2 & corner_a8) || (promosq1 == H1 && bbk2 & corner_h1) || (promosq1 == A1 && bbk2 & corner_a1))
      return draw_score(m);
  }

  return eval;
}

[[nodiscard]]
i32 KBxKx(Material *m, const i32 eval, const u32 key1, const u32 key2, const Color c1)
{
  return (key1 & all_pawns) == 1 && (key2 & all_pawns) == 0 ? KBpK(m, eval, c1) : eval;
}

[[nodiscard]]
i32 KBxKX(Material *m, const i32 eval, const u32 key1, const u32 key2, const Color c1)
{
  switch (key2 & ~all_pawns)
  {
  case kb:
    if (!same_color(lsb(m->board->pieces(BISHOP, WHITE)), lsb(m->board->pieces(BISHOP, BLACK))) && util::abs(pawn_count(m, WHITE) - pawn_count(m, BLACK)) <= 2)
      return eval / 2;

    break;

  case k:
    return KBxKx(m, eval, key1, key2, c1);

  default:
    break;
  }
  return eval;
}

[[nodiscard]]
i32 KpK(Material *m, const i32 eval, const Color c1)
{
  const Square pawnsq1  = lsb(m->board->pieces(PAWN, c1));
  const Square promosq1 = static_cast<Square>(c1 == BLACK ? file_of(pawnsq1) : file_of(pawnsq1) + 56);
  const Bitboard bbk2   = m->board->pieces(KING, ~c1);

  return (promosq1 == H8 && bbk2 & corner_h8) || (promosq1 == A8 && bbk2 & corner_a8) || (promosq1 == H1 && bbk2 & corner_h1) || (promosq1 == A1 && bbk2 & corner_a1) ? draw_score(m) : eval;
}

[[nodiscard]]
i32 KxKx(Material *m, const i32 eval, const i32 pc1, const i32 pc2, const Color c1)
{
  return pc1 == 1 && pc2 == 0 ? KpK(m, eval, c1) : eval;
}

[[nodiscard]]
i32 KKx(Material *m, const i32 eval, const i32 pc1, const i32 pc2, const Color c1)
{
  return pc1 + pc2 == 0 ? draw_score(m) : pc2 > 0 ? KxKx(m, eval, pc1, pc2, c1) : eval;
}

[[nodiscard]]
i32 KBKX(Material *m, const i32 eval, const u32 key1, const u32 key2, const i32 pc1, const i32 pc2, const Color c1, const Color c2, const Color c)
{
  if (pc1 > 0)
    return KBxKX(m, eval, key1, key2, c1);

  switch (key2 & ~all_pawns)
  {
  case k: {
    if (pc1 + pc2 == 0)
      return draw_score(m);
    if (pc1 == 0 && pc2 == 1)
    {
      if (c1 == c || !m->board->is_attacked(lsb(m->board->pieces(BISHOP, c1)), c2))
      {
        const Bitboard bishopbb = m->board->pieces(BISHOP, c1);
        if (pawn_front_spanBB(c2, lsb(m->board->pieces(PAWN, c2))) & (piece_attacks_bb<BISHOP>(lsb(bishopbb), m->board->pieces()) | bishopbb))
          return draw_score(m);
      }
    }
    break;
  }

  case kb:
  case knn:
  case kn:
    m->drawish = 16;
    break;

  default:
    break;
  }
  return std::min(0, eval);
}

}   // namespace

namespace material
{

void clear(Material *m)
{
  m->key[0] = m->key[1] = 0;
  m->material_value[0] = m->material_value[1] = 0;
}

void remove(Material *m, const Piece pc)
{
  const Color c      = color_of(pc);
  const PieceType pt = type_of(pc);
  update_key(m, c, pt, -1);
  m->material_value[c] -= piece_values[pt];
}

void add(Material *m, const Piece pc)
{
  const Color c      = color_of(pc);
  const PieceType pt = type_of(pc);
  update_key(m, c, pt, 1);
  m->material_value[c] += piece_values[pt];
}

i32 count(const Material *m, const Color c, const PieceType pt)
{
  return m->key[c] >> piece_bit_shift[pt] & 15;
}

void make_move(Material *m, const Move move)
{
  if (is_capture(move))
    remove(m, move_captured(move));

  if (is_promotion(move))
  {
    remove(m, move_piece(move));
    add(m, move_promoted(move));
  }
}

bool is_kx(const Material *m, const Color c)
{
  return m->key[c] == (m->key[c] & 15);
}

i32 value(const Material *m)
{
  return m->material_value[WHITE] + m->material_value[BLACK];
}

i32 pawn_value(const Material *m)
{
  return static_cast<i32>(m->key[WHITE] & all_pawns) * piece_values[PAWN] + static_cast<i32>(m->key[BLACK] & all_pawns) * piece_values[PAWN];
}

i32 pawn_count(const Material *m)
{
  return static_cast<i32>(m->key[WHITE] & 15) + static_cast<i32>(m->key[BLACK] & 15);
}

i32 evaluate(Material *m, i32 &flags, const int eval, const Board *b, const Color us)
{
  const Color them  = ~us;
  m->board          = b;
  m->drawish        = 0;
  m->material_flags = 0;

  u32 strong_key;
  u32 weak_key;
  i32 score;
  Color strong_side;

  if (m->key[us] >= m->key[them])
  {
    strong_key  = m->key[us];
    weak_key    = m->key[them];
    strong_side = us;
    score       = eval;
  } else
  {
    strong_key  = m->key[them];
    weak_key    = m->key[us];
    strong_side = them;
    score       = -eval;
  }

  const Color weak_side        = ~strong_side;
  const i32 strong_pawn_count = ::pawn_count(m, strong_side);
  const i32 weak_pawn_count   = ::pawn_count(m, weak_side);

  switch (strong_key & ~all_pawns)
  {
  case kqb:
    score = KQBKX(m, score, weak_key);
    break;

  case kqn:
    score = KQNKX(m, score, weak_key);
    break;

  case krb:
    score = KRBKX(m, score, weak_key);
    break;

  case krn:
    score = KRNKX(m, score, weak_key);
    break;

  case kr:
    score = KRKX(m, score, weak_key);
    break;

  case kbb:
    score = KBBKX(m, score, weak_key);
    break;

  case kbn:
    score = KBNKX(m, score, weak_key, strong_pawn_count, weak_pawn_count, strong_side);
    break;

  case kb:
    score = KBKX(m, score, strong_key, weak_key, strong_pawn_count, weak_pawn_count, strong_side, weak_side, us);
    break;

  case kn:
    score = KNKX(m, score, weak_key, strong_pawn_count, weak_pawn_count, strong_side, weak_side, us);
    break;

  case knn:
    score = KNNKX(m, score, weak_key, strong_pawn_count);
    break;

  case k:
    score = KKx(m, score, strong_pawn_count, weak_pawn_count, strong_side);
    break;

  default:
    break;
  }

  if (m->drawish)
  {
    if (const auto drawish_score = score / m->drawish; strong_pawn_count + weak_pawn_count == 0)
      score = drawish_score;
    else if (strong_pawn_count == 0)
      score = std::min<int>(drawish_score, score);
    else if (weak_pawn_count == 0)
      score = std::max<int>(drawish_score, score);
  }

  flags = m->material_flags;
  return strong_side != us ? -score : score;
}

}   // namespace material

// Feliscatus, a UCI chess playing engine derived from Tomcat 1.0 (Bobcat 8.0)
// Copyright (C) 2008-2016 Gunnar Harms (Bobcat author)
// Copyright (C) 2017      FireFather (Tomcat author)
// Copyright (C) 2020-2022 Rudy Alex Kohn
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