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

#include "moves.hpp"
#include "board.hpp"

namespace
{

constexpr std::array<PieceType, 5> MoveGenPieceTypes{QUEEN, ROOK, BISHOP, KNIGHT, KING};

template<Color Us>
[[nodiscard]]
bool can_castle_short(Board *b)
{
  return b->can_castle(make_castling<Us, KING_SIDE>()) && b->is_castleling_impeeded(oo_king_to[Us], Us);
}

template<Color Us>
[[nodiscard]]
bool can_castle_long(Board *b)
{
  return b->can_castle(make_castling<Us, QUEEN_SIDE>()) && b->is_castleling_impeeded(ooo_king_to[Us], Us);
}

}   // namespace

namespace MoveGen
{

template<MoveGenFlags Flags, Color Us>
[[nodiscard]]
MoveData *add_move(
  Board *b, const Piece pc, const Square from, const Square to, const MoveType mt, MoveData *md,
  const PieceType promo_pt = NO_PT)
{

  const auto captured = [&mt, &b, &to]() {
    if (mt & CAPTURE)
      return b->piece(to);
    if (mt & EPCAPTURE)
      return make_piece(PAWN, ~Us);
    return NO_PIECE;
  };

  const auto captured_piece = captured();
  const auto move           = init_move(pc, captured_piece, from, to, mt, make_piece(promo_pt, Us));

  // if constexpr (Flags & LEGALMOVES)
  if (!b->is_legal(move, pc, from, mt))
    return md;

  *md++ = move;

  return md;
}

template<MoveGenFlags Flags, Color Us>
[[nodiscard]]
MoveData *add_moves(Board *b, const PieceType pt, const Square from, const Bitboard attacks, MoveData *md)
{
  const auto pc = make_piece(pt, Us);

  for (auto bb = attacks; bb;)
  {
    const auto to = pop_lsb(&bb);
    md            = add_move<Flags, Us>(b, pc, from, to, b->piece(to) == NO_PIECE ? NORMAL : CAPTURE, md);
  }

  return md;
}

template<MoveGenFlags Flags, Color Us>
[[nodiscard]]
MoveData *add_moves(Board *b, const Bitboard to_squares, MoveData *md)
{
  const auto pieces = b->pieces();

  for (const auto pt : MoveGenPieceTypes)
  {
    auto bb = b->pieces(pt, Us);
    while (bb)
    {
      const auto from = pop_lsb(&bb);
      md              = add_moves<Flags, Us>(b, pt, from, piece_attacks_bb(pt, from, pieces) & to_squares, md);
    }
  }

  return md;
}

template<MoveGenFlags Flags, Color Us>
[[nodiscard]]
MoveData *add_castle_move(Board *b, const Square from, const Square to, MoveData *md)
{
  return add_move<Flags, Us>(b, make_piece(KING, Us), from, to, CASTLE, md);
}

template<MoveGenFlags Flags, Color Us, MoveType Type, Direction D>
[[nodiscard]]
MoveData *add_pawn_moves(Board *b, const Bitboard to_squares, MoveData *md)
{
  constexpr auto Rank8 = bit(relative_rank(Us, RANK_8));
  const auto pawn      = make_piece(PAWN, Us);

  Bitboard targets;

  // promotion moves
  if constexpr (Type != DOUBLEPUSH || Type != EPCAPTURE)
  {
    targets = to_squares & Rank8;

    while (targets)
    {
      const auto to             = pop_lsb(&targets);
      const auto from           = to - D;
      constexpr auto promo_type = Type | PROMOTION;
      for (const auto promoted : PromotionPieceTypes)
        md = add_move<Flags, Us>(b, pawn, from, to, promo_type, md, promoted);
    }
  }

  // non-promotion moves

  targets = to_squares & ~Rank8;

  while (targets)
  {
    const auto to = pop_lsb(&targets);
    md            = add_move<Flags, Us>(b, pawn, to - D, to, Type, md);
  }

  return md;
}

template<MoveGenFlags Flags, Color Us>
[[nodiscard]]
MoveData *generate_pawn_moves(Board *b, MoveData *md, const Bitboard targets)
{

  constexpr auto pc   = make_piece(PAWN, Us);
  constexpr auto Them = ~Us;
  constexpr auto Up   = pawn_push(Us);

  const auto pawns = [&b]() {
    constexpr auto Rank7 = rank_7[Us];
    const auto our_pawns = b->pieces(PAWN, Us);
    return std::make_pair(our_pawns & Rank7, our_pawns & ~Rank7);
  };

  const auto [promotion_pawns, non_promotion_pawns] = pawns();

  Bitboard not_occupied, opponents;

  if constexpr (Flags == CAPTURES)
    opponents = targets;
  else
    opponents = b->pieces(Them);

  if constexpr (Flags == QUIET)
  {
    constexpr auto Rank3 = rank_3[Us];
    not_occupied         = targets;

    auto pawn_single_push = shift_bb<Up>(non_promotion_pawns) & not_occupied;
    auto pawn_double_push = shift_bb<Up>(pawn_single_push & Rank3) & not_occupied;

    while (pawn_single_push)
    {
      const auto to = pop_lsb(&pawn_single_push);
      *md++         = init_move<NORMAL>(pc, NO_PIECE, to - Up, to, NO_PIECE);
    }

    while (pawn_double_push)
    {
      const auto to = pop_lsb(&pawn_double_push);
      *md++         = init_move<DOUBLEPUSH>(pc, NO_PIECE, to - (Up * 2), to, NO_PIECE);
    }
  }

  if (promotion_pawns)
  {
    // handle captures
    if constexpr (Flags == CAPTURES)
    {
      constexpr auto NorthEast = pawn_east_attack_dist[Us];
      constexpr auto NorthWest = pawn_west_attack_dist[Us];

      not_occupied       = ~b->pieces();
      auto pawns_up_east = shift_bb<NorthEast>(promotion_pawns) & opponents;
      auto pawns_up_west = shift_bb<NorthWest>(promotion_pawns) & opponents;

      while (pawns_up_east)
      {
        const auto to = pop_lsb(&pawns_up_east);
        md            = add_move<Flags, Us>(b, pc, to - NorthEast, to, PROMOTION | CAPTURE, md, QUEEN);
        md            = add_move<Flags, Us>(b, pc, to - NorthEast, to, PROMOTION | CAPTURE, md, ROOK);
        md            = add_move<Flags, Us>(b, pc, to - NorthEast, to, PROMOTION | CAPTURE, md, ROOK);
        md            = add_move<Flags, Us>(b, pc, to - NorthEast, to, PROMOTION | CAPTURE, md, KNIGHT);
      }

      while (pawns_up_west)
      {
        const auto to = pop_lsb(&pawns_up_west);
        md            = add_move<Flags, Us>(b, pc, to - NorthWest, to, PROMOTION | CAPTURE, md, QUEEN);
        md            = add_move<Flags, Us>(b, pc, to - NorthWest, to, PROMOTION | CAPTURE, md, ROOK);
        md            = add_move<Flags, Us>(b, pc, to - NorthWest, to, PROMOTION | CAPTURE, md, ROOK);
        md            = add_move<Flags, Us>(b, pc, to - NorthWest, to, PROMOTION | CAPTURE, md, KNIGHT);
      }
    } else if constexpr (Flags == QUIET)
    {
      auto pawns_up = shift_bb<Up>(promotion_pawns) & not_occupied;
      while (pawns_up)
      {
        const auto to = pop_lsb(&pawns_up);
        md            = add_move<Flags, Us>(b, pc, to - Up, to, PROMOTION, md, QUEEN);
        md            = add_move<Flags, Us>(b, pc, to - Up, to, PROMOTION, md, ROOK);
        md            = add_move<Flags, Us>(b, pc, to - Up, to, PROMOTION, md, BISHOP);
        md            = add_move<Flags, Us>(b, pc, to - Up, to, PROMOTION, md, KNIGHT);
      }
    }
  }

  return md;
}

template<MoveGenFlags Flags, Color Us>
[[nodiscard]]
MoveData *generate_quiet_moves(Board *b, MoveData *md)
{
  constexpr auto NotRank7  = ~rank_7[Us];
  constexpr auto Rank3     = rank_3[Us];
  constexpr auto Up        = pawn_push(Us);
  const auto empty_squares = ~b->pieces();
  const auto pushed        = shift_bb<Up>(b->pieces(PAWN, Us) & NotRank7) & empty_squares;

  // md = generate_pawn_moves<QUIET, Us>(b, md, empty_squares);
  md = add_pawn_moves<Flags, Us, NORMAL, Up>(b, pushed, md);
  md = add_pawn_moves<Flags, Us, DOUBLEPUSH, Up * 2>(b, shift_bb<Up>(pushed & Rank3) & empty_squares, md);
  md = add_moves<Flags, Us>(b, empty_squares, md);

  if (b->in_check())
    return md;

  if (can_castle_short<Us>(b))
    md = add_castle_move<Flags, Us>(b, oo_king_from[Us], oo_king_to[Us], md);

  if (can_castle_long<Us>(b))
    md = add_castle_move<Flags, Us>(b, ooo_king_from[Us], ooo_king_to[Us], md);

  return md;
}

template<MoveGenFlags Flags, Color Us>
[[nodiscard]]
MoveData *generate_captures_and_promotions(Board *b, MoveData *md)
{
  constexpr auto Them         = ~Us;
  constexpr auto WestAttacks  = pawn_west_attacks[Us];
  constexpr auto EastAttacks  = pawn_east_attacks[Us];
  constexpr auto WestDistance = pawn_west_attack_dist[Us];
  constexpr auto EastDistance = pawn_east_attack_dist[Us];
  constexpr auto Rank_7       = rank_7[Us];
  constexpr auto Up           = pawn_push(Us);
  const auto opponent_pieces  = b->pieces(Them);
  const auto pawns            = b->pieces(PAWN, Us);

  md = add_pawn_moves<Flags, Us, NORMAL, Up>(b, pawn_push(Us, pawns & Rank_7) & ~b->pieces(), md);
  md = add_pawn_moves<Flags, Us, CAPTURE, WestDistance>(b, WestAttacks(pawns) & opponent_pieces, md);
  md = add_pawn_moves<Flags, Us, CAPTURE, EastDistance>(b, EastAttacks(pawns) & opponent_pieces, md);

  [[unlikely]]
  if (b->en_passant_square() != NO_SQ)
  {
    md = add_pawn_moves<Flags, Us, EPCAPTURE, WestDistance>(b, WestAttacks(pawns) & b->en_passant_square(), md);
    md = add_pawn_moves<Flags, Us, EPCAPTURE, EastDistance>(b, EastAttacks(pawns) & b->en_passant_square(), md);
  }

  return add_moves<Flags, Us>(b, opponent_pieces, md);
}

template<Color Us, MoveGenFlags Flags>
[[nodiscard]]
MoveData *generate_all_moves(Board *b, MoveData *md)
{
  constexpr auto Them = ~Us;

  Bitboard targets;

  if constexpr (Flags == CAPTURES)
    targets = b->pieces(Them);
  else if constexpr (Flags == QUIET)
    targets = ~b->pieces();

  // TODO : Finish

  return md;
}

template<>
[[nodiscard]]
MoveData *generate<CAPTURES>(Board *b, MoveData *md)
{
  const auto c = b->side_to_move();
  return c == WHITE ? generate_captures_and_promotions<LEGALMOVES, WHITE>(b, md)
                    : generate_captures_and_promotions<LEGALMOVES, BLACK>(b, md);
}

template<>
[[nodiscard]]
MoveData *generate<QUIET>(Board *b, MoveData *md)
{
  const auto c = b->side_to_move();
  return c == WHITE ? generate_quiet_moves<QUIET, WHITE>(b, md) : generate_quiet_moves<QUIET, BLACK>(b, md);
}

template<MoveGenFlags Flags>
[[nodiscard]]
MoveData *generate(Board *b, MoveData *md)
{
  static_assert(Flags == LEGALMOVES);

  md = generate<CAPTURES>(b, md);
  md = generate<QUIET>(b, md);
  return md;
}

template MoveData *generate<LEGALMOVES>(Board *, MoveData *);

}   // namespace MoveGen
