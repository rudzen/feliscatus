
#pragma once

#include <cstdint>
#include <array>
#include "bitboard.h"
#include "move.h"
#include "magic.h"

class Board {
public:
  void clear() {
    piece.fill(0);
    occupied_by_side.fill(0);
    board.fill(NoPiece);
    king_square.fill(no_square);
    occupied = 0;
  }

  void add_piece(const int p, const Color side, const Square sq) {
    piece[p + (side << 3)] |= sq;
    occupied_by_side[side] |= sq;
    occupied |= sq;
    board[sq] = p + (side << 3);

    if (p == King)
      king_square[side] = sq;
  }

  void remove_piece(const int p, const Square sq) {
    const auto bbsq = bit(sq);
    piece[p] &= ~bbsq;
    occupied_by_side[p >> 3] &= ~bbsq;
    occupied &= ~bbsq;
    board[sq] = NoPiece;
  }

  void add_piece(const int p, const Square sq) {
    piece[p] |= sq;
    occupied_by_side[p >> 3] |= sq;
    occupied |= sq;
    board[sq] = p;
  }

  void make_move(const uint32_t m) {

    const auto from = move_from(m);
    const auto to   = move_to(m);
    const auto pc   = move_piece(m);

    if (!is_castle_move(m))
    {
      remove_piece(pc, from);

      if (is_ep_capture(m))
      {
        if (pc < 8)
          remove_piece(moveCaptured(m), static_cast<Square>(to - 8));
        else
          remove_piece(moveCaptured(m), static_cast<Square>(to + 8));
      } else if (is_capture(m))
        remove_piece(moveCaptured(m), to);

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

    if ((move_piece(m) & 7) == King)
      king_square[move_side(m)] = move_to(m);
  }

  void unmake_move(const uint32_t m) {

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
        if (pc < 8)
          add_piece(moveCaptured(m), static_cast<Square>(to - 8));
        else
          add_piece(moveCaptured(m), static_cast<Square>(to + 8));
      } else if (is_capture(m))
        add_piece(moveCaptured(m), to);

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

  [[nodiscard]]
  int get_piece(const Square sq) const { return board[sq]; }

  [[nodiscard]]
  int get_piece_type(const Square sq) const { return board[sq] & 7; }

  [[nodiscard]]
  Bitboard get_pinned_pieces(const Color side, const Square sq) {
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

  static Bitboard xray_rook_attacks(const Bitboard occ, Bitboard blockers, const Square sq) {
    const auto attacks = rookAttacks(sq, occ);
    blockers &= attacks;
    return attacks ^ rookAttacks(sq, occ ^ blockers);
  }

  static Bitboard xray_bishop_attacks(const Bitboard occ, Bitboard blockers, const Square sq) {
    const auto attacks = bishopAttacks(sq, occ);
    blockers &= attacks;
    return attacks ^ bishopAttacks(sq, occ ^ blockers);
  }

  [[nodiscard]]
  bool is_occupied(const Square sq) const { return occupied & sq; }

  [[nodiscard]]
  bool is_attacked(const Square sq, const Color side) const {
    return is_attacked_by_slider(sq, side) || is_attacked_by_knight(sq, side) || is_attacked_by_pawn(sq, side) || is_attacked_by_king(sq, side);
  }

  [[nodiscard]]
  Bitboard piece_attacks(const int pc, const Square sq) const {
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

  [[nodiscard]]
  bool is_attacked_by_slider(const Square sq, const Color side) const {
    const auto r_attacks = rookAttacks(sq, occupied);

    if (piece[Rook + (side << 3)] & r_attacks)
      return true;

    const auto b_attacks = bishopAttacks(sq, occupied);

    if (piece[Bishop + (side << 3)] & b_attacks)
      return true;

    if (piece[Queen + (side << 3)] & (b_attacks | r_attacks))
    { return true; }
    return false;
  }

  [[nodiscard]]
  bool is_attacked_by_knight(const Square sq, const Color side) const { return (piece[Knight + (side << 3)] & knight_attacks[sq]) != 0; }

  [[nodiscard]]
  bool is_attacked_by_pawn(const Square sq, const Color side) const { return (piece[Pawn | side << 3] & pawn_captures[sq | (~side) << 6]) != 0; }

  [[nodiscard]]
  bool is_attacked_by_king(const Square sq, const Color side) const { return (piece[King | side << 3] & king_attacks[sq]) != 0; }

  void print() const {
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

  [[nodiscard]]
  Bitboard pieces(const int p, const Color side) const { return piece[p | side << 3]; }

  [[nodiscard]]
  Bitboard pawns(const Color side) const { return piece[Pawn | side << 3]; }

  [[nodiscard]]
  Bitboard knights(const Color side) const { return piece[Knight | side << 3]; }

  [[nodiscard]]
  Bitboard bishops(const Color side) const { return piece[Bishop | side << 3]; }

  [[nodiscard]]
  Bitboard rooks(const Color side) const { return piece[Rook | side << 3]; }

  [[nodiscard]]
  Bitboard queens(const Color side) const { return piece[Queen | side << 3]; }

  [[nodiscard]]
  Bitboard king(const Color side) const { return piece[King | side << 3]; }

  std::array<Bitboard, 2 << 3> piece{};
  std::array<Bitboard, COL_NB> occupied_by_side{};
  std::array<int, 64> board{};
  std::array<Square, COL_NB> king_square{};
  Bitboard queen_attacks{};
  Bitboard occupied{};

  [[nodiscard]]
  bool is_pawn_passed(const Square sq, const Color side) const { return (passed_pawn_front_span[side][sq] & pawns(~side)) == 0; }

  [[nodiscard]]
  bool is_piece_on_square(const int p, const Square sq, const Color side) { return (piece[p + (side << 3)] & sq) != 0; }

  [[nodiscard]]
  bool is_piece_on_file(const int p, const Square sq, const Color side) const { return (bb_file(sq) & piece[p + (side << 3)]) != 0; }

  [[nodiscard]]
  bool is_pawn_isolated(const Square sq, const Color side) const {
    const auto bb              = bit(sq);
    const auto neighbour_files = north_fill(south_fill(west_one(bb) | east_one(bb)));
    return (pawns(side) & neighbour_files) == 0;
  }

  [[nodiscard]]
  bool is_pawn_behind(const Square sq, const Color side) const {
    const auto bbsq = bit(sq);
    return (pawns(side) & pawn_fill[~side](west_one(bbsq) | east_one(bbsq))) == 0;
  }
};