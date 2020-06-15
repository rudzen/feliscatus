#include "board.h"
#include "magic.h"
#include "move.h"

void Board::clear() {
  piece.fill(0);
  occupied_by_side.fill(0);
  board.fill(NoPiece);
  king_square.fill(no_square);
  occupied = 0;
}

void Board::make_move(const uint32_t m) {

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
    remove_piece(Rook + side_mask(m), rook_castles_from[to]);
    remove_piece(pc, from);
    add_piece(Rook + side_mask(m), rook_castles_to[to]);
    add_piece(pc, to);
  }

  if ((pc & 7) == King)
    king_square[move_side(m)] = to;
}

void Board::unmake_move(const uint32_t m) {

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
    remove_piece(pc, to);
    remove_piece(Rook + side_mask(m), rook_castles_to[to]);
    add_piece(pc, from);
    add_piece(Rook + side_mask(m), rook_castles_from[to]);
  }

  if ((pc & 7) == King)
    king_square[move_side(m)] = from;
}

Bitboard Board::get_pinned_pieces(const Color side, const Square sq) {
  Bitboard pinned_pieces = 0;
  const auto opp         = ~side;
  auto pinners           = xray_bishop_attacks(occupied, occupied_by_side[side], sq) & (piece[Bishop + (opp << 3)] | piece[Queen | opp << 3]);

  while (pinners)
  {
    pinned_pieces |= between_bb[lsb(pinners)][sq] & occupied_by_side[side];
    reset_lsb(pinners);
  }
  pinners = xray_rook_attacks(occupied, occupied_by_side[side], sq) & (piece[Rook + (opp << 3)] | piece[Queen | opp << 3]);

  while (pinners)
  {
    pinned_pieces |= between_bb[lsb(pinners)][sq] & occupied_by_side[side];
    reset_lsb(pinners);
  }
  return pinned_pieces;
}

Bitboard Board::piece_attacks(const int pc, const Square sq) const {
  switch (pc & 7)
  {
  case Knight:
    return knightAttacks(sq);

  case Bishop:
    return bishopAttacks(sq, occupied);

  case Rook:
    return rookAttacks(sq, occupied);

  case Queen:
    return queenAttacks(sq, occupied);

  case King:
    return kingAttacks(sq);

  default:
    break;// error
  }
  return 0;
}

bool Board::is_attacked_by_slider(const Square sq, const Color side) const {
  const auto r_attacks = rookAttacks(sq, occupied);

  if (piece[Rook + (side << 3)] & r_attacks)
    return true;

  const auto b_attacks = bishopAttacks(sq, occupied);

  if (piece[Bishop + (side << 3)] & b_attacks)
    return true;

  if (piece[Queen + (side << 3)] & (b_attacks | r_attacks))
    return true;
  return false;
}

void Board::print() const {
  constexpr std::string_view piece_letter = "PNBRQK. pnbrqk. ";
  printf("\n");

  for (const Rank rank : ReverseRanks)
  {
    printf("%d  ", rank + 1);

    for (const auto file : Files)
    {
      const auto sq = make_square(file, rank);
      const auto pc = get_piece(sq);
      printf("%c ", piece_letter[pc]);
    }
    printf("\n");
  }
  printf("   a b c d e f g h\n");
}

bool Board::is_pawn_isolated(const Square sq, const Color side) const {
  const auto bb              = bit(sq);
  const auto neighbour_files = north_fill(south_fill(west_one(bb) | east_one(bb)));
  return (pawns(side) & neighbour_files) == 0;
}

bool Board::is_pawn_behind(const Square sq, const Color side) const {
  const auto bbsq = bit(sq);
  return (pawns(side) & pawn_fill[~side](west_one(bbsq) | east_one(bbsq))) == 0;
}
