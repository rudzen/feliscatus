#include <fmt/format.h>
#include "board.h"
#include "magic.h"

void Board::clear() {
  piece.fill(0);
  occupied_by_side.fill(0);
  board.fill(NoPiece);
  king_square.fill(no_square);
  occupied = 0;
}

void Board::make_move(const Move m) {

  const auto from = move_from(m);
  const auto to   = move_to(m);
  const auto pc   = move_piece(m);

  if (!is_castle_move(m))
  {
    remove_piece(pc, from);

    if (is_ep_capture(m))
    {
      const auto direction = pc < 8 ? SOUTH : NORTH;
      remove_piece(move_captured(m), to + direction);
    } else if (is_capture(m))
      remove_piece(move_captured(m), to);

    if (is_promotion(m))
      add_piece(move_promoted(m), to);
    else
      add_piece(pc, to);
  } else
  {
    const auto rook = Rook | side_mask(m);
    remove_piece(rook, rook_castles_from[to]);
    remove_piece(pc, from);
    add_piece(rook, rook_castles_to[to]);
    add_piece(pc, to);
  }

  if ((pc & 7) == King)
    king_square[move_side(m)] = to;
}

void Board::unmake_move(const Move m) {

  const auto from = move_from(m);
  const auto to   = move_to(m);
  const auto pc   = move_piece(m);

  if (!is_castle_move(m))
  {
    if (is_promotion(m))
      remove_piece(move_promoted(m), to);
    else
      remove_piece(pc, to);

    if (is_ep_capture(m))
    {
      const auto direction = pc < 8 ? SOUTH : NORTH;
      add_piece(move_captured(m), to + direction);
    } else if (is_capture(m))
      add_piece(move_captured(m), to);

    add_piece(pc, from);
  } else
  {
    const auto rook = Rook | side_mask(m);
    remove_piece(pc, to);
    remove_piece(rook, rook_castles_to[to]);
    add_piece(pc, from);
    add_piece(rook, rook_castles_from[to]);
  }

  if ((pc & 7) == King)
    king_square[move_side(m)] = from;
}

Bitboard Board::get_pinned_pieces(const Color side, const Square sq) {
  Bitboard pinned_pieces = 0;
  const auto opp_mask    = (~side) << 3;
  auto pinners           = xray_bishop_attacks(occupied, occupied_by_side[side], sq) & (piece[Bishop | opp_mask] | piece[Queen | opp_mask]);

  while (pinners)
  {
    pinned_pieces |= between_bb[lsb(pinners)][sq] & occupied_by_side[side];
    reset_lsb(pinners);
  }
  pinners = xray_rook_attacks(occupied, occupied_by_side[side], sq) & (piece[Rook | opp_mask] | piece[Queen | opp_mask]);

  while (pinners)
  {
    pinned_pieces |= between_bb[lsb(pinners)][sq] & occupied_by_side[side];
    reset_lsb(pinners);
  }
  return pinned_pieces;
}

bool Board::is_attacked_by_slider(const Square sq, const Color side) const {
  const auto mask = side << 3;
  const auto r_attacks = piece_attacks_bb<Rook>(sq, occupied);

  if (piece[Rook | mask] & r_attacks)
    return true;

  const auto b_attacks = piece_attacks_bb<Bishop>(sq, occupied);

  if (piece[Bishop | mask] & b_attacks)
    return true;

  return (piece[Queen | mask] & (b_attacks | r_attacks)) != 0;
}

void Board::print() const {
  constexpr std::string_view piece_letter = "PNBRQK. pnbrqk. ";

  fmt::memory_buffer s;

  fmt::format_to(s, "\n");

  for (const Rank rank : ReverseRanks)
  {
    fmt::format_to(s, "{}  ", rank + 1);

    for (const auto file : Files)
    {
      const auto sq = make_square(file, rank);
      const auto pc = get_piece(sq);
      fmt::format_to(s, "{} ", piece_letter[pc]);
    }
    fmt::format_to(s, "\n");
  }

  fmt::print("{}   a b c d e f g h\n", fmt::to_string(s));
}

bool Board::is_passed_pawn_move(const Move m) const {
  return move_piece_type(m) == Pawn && is_pawn_passed(move_to(m), move_side(m));
}

bool Board::is_pawn_isolated(const Square sq, const Color side) const {
  const auto f               = bb_file(file_of(sq));
  const auto neighbour_files = east_one(f) | west_one(f);
  return (pieces(Pawn, side) & neighbour_files) == 0;
}

bool Board::is_pawn_behind(const Square sq, const Color side) const {
  const auto bbsq = bit(sq);
  return (pieces(Pawn, side) & pawn_fill[~side](west_one(bbsq) | east_one(bbsq))) == 0;
}
