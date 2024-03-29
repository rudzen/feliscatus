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

#include <optional>

#include "board.hpp"

namespace
{

using CurrentPieceType = std::array<PieceType, COL_NB>;
using CurrentPieces    = std::array<Bitboard, COL_NB>;

struct SeeData final
{
  CurrentPieceType current_pt{};
  CurrentPieces current_pc{};
};

constexpr int SEE_INVALID_SCORE = -5000;

[[nodiscard]]
constexpr auto material_change(const Move m)
{
  return (is_capture(m) ? piece_value(move_captured(m)) : 0)
         + (is_promotion(m) ? (piece_value(move_promoted(m)) - piece_value<PAWN>()) : 0);
}

[[nodiscard]]
constexpr auto next_to_capture(const Move m)
{
  return is_promotion(m) ? move_promoted(m) : move_piece(m);
}

[[nodiscard]]
auto from_sq(const Bitboard bb, SeeData &data, const Color c)
{
  const auto from = lsb(bb);
  data.current_pc[c] &= ~bit(from);
  return std::make_optional(from);
}

/// "Best" == "Lowest piece value"
[[nodiscard]]
std::optional<Square> lookup_best_attacker(SeeData &data, const Square to, const Color c, const Board *b)
{
  const auto occupied = b->pieces();
  Bitboard bb;

  switch (data.current_pt[c])
  {
  case PAWN:
    bb = data.current_pc[c] & pawn_attacks_bb(~c, to);
    if (bb)
      return from_sq(bb, data, c);
    data.current_pc[c] = b->pieces(++data.current_pt[c], c);
    [[fallthrough]];

  case KNIGHT:
    bb = data.current_pc[c] & piece_attacks_bb<KNIGHT>(to);
    if (bb)
      return from_sq(bb, data, c);
    data.current_pc[c] = b->pieces(++data.current_pt[c], c);
    [[fallthrough]];

  case BISHOP:
    bb = data.current_pc[c] & piece_attacks_bb<BISHOP>(to, occupied);
    if (bb)
      return from_sq(bb, data, c);
    data.current_pc[c] = b->pieces(++data.current_pt[c], c);
    [[fallthrough]];

  case ROOK:
    bb = data.current_pc[c] & piece_attacks_bb<ROOK>(to, occupied);
    if (bb)
      return from_sq(bb, data, c);
    data.current_pc[c] = b->pieces(++data.current_pt[c], c);
    [[fallthrough]];

  case QUEEN:
    bb = data.current_pc[c] & piece_attacks_bb<QUEEN>(to, occupied);
    if (bb)
      return from_sq(bb, data, c);
    data.current_pc[c] = b->pieces(++data.current_pt[c], c);
    [[fallthrough]];

  case KING:
    bb = data.current_pc[c] & piece_attacks_bb<KING>(to);
    if (bb)
      return from_sq(bb, data, c);
    break;
  default:
    break;
  }

  return std::nullopt;
}

}   // namespace

int Board::see_move(const Move m)
{
  perform_move(m);

  const auto us    = move_side(m);
  const auto them  = ~us;
  const auto score = !is_attacked(square<KING>(us), them) ? see_rec(material_change(m), next_to_capture(m), move_to(m), them)
                                                     : SEE_INVALID_SCORE;

  unperform_move(m);
  return score;
}

int Board::see_last_move(const Move m)
{
  return see_rec(material_change(m), next_to_capture(m), move_to(m), ~move_side(m));
}

int Board::see_rec(const int mat_change, const Piece next_capture, const Square to, const Color c)
{
  SeeData data{{PAWN, PAWN}, {pieces(PAWN, WHITE), pieces(PAWN, BLACK)}};
  const auto rr = relative_rank(c, to);
  Move m;

  do
  {
    const auto from = lookup_best_attacker(data, to, c, this);
    if (!from)
      return mat_change;

    const auto current_pt = data.current_pt[c];

    m = current_pt == PAWN && rr == RANK_8
          ? init_move<PROMOTION | CAPTURE>(
            make_piece(current_pt, c), next_capture, from.value(), to, make_piece(QUEEN, c))
          : init_move<CAPTURE>(make_piece(current_pt, c), next_capture, from.value(), to, NO_PIECE);

    perform_move(m);

    if (!is_attacked(square<KING>(c), ~c))
      break;

    unperform_move(m);
  } while (true);

  const auto score = -see_rec(material_change(m), next_to_capture(m), move_to(m), ~move_side(m));

  unperform_move(m);

  return score < 0 ? mat_change + score : mat_change;
}
