
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
    king_square.fill(64);
    occupied = 0;
  }

  void add_piece(const int p, const int side, const uint64_t sq) {
    piece[p + (side << 3)] |= bb_square(sq);
    occupied_by_side[side] |= bb_square(sq);
    occupied |= bb_square(sq);
    board[sq] = p + (side << 3);

    if (p == King)
    {
      king_square[side] = sq;
    }
  }

  void remove_piece(const int p, const int sq) {
    piece[p] &= ~bb_square(sq);
    occupied_by_side[p >> 3] &= ~bb_square(sq);
    occupied &= ~bb_square(sq);
    board[sq] = NoPiece;
  }

  void add_piece(const int p, const int sq) {
    piece[p] |= bb_square(sq);
    occupied_by_side[p >> 3] |= bb_square(sq);
    occupied |= bb_square(sq);
    board[sq] = p;
  }

  void make_move(const uint32_t m) {
    if (!is_castle_move(m))
    {
      remove_piece(move_piece(m), move_from(m));

      if (is_ep_capture(m))
      {
        if (move_piece(m) < 8)
          remove_piece(moveCaptured(m), move_to(m) - 8);
        else
          remove_piece(moveCaptured(m), move_to(m) + 8);
      } else if (is_capture(m))
        remove_piece(moveCaptured(m), move_to(m));

      if (move_type(m) & PROMOTION)
        add_piece(move_promoted(m), move_to(m));
      else
        add_piece(move_piece(m), move_to(m));
    } else
    {
      remove_piece(Rook + side_mask(m), rook_castles_from[move_to(m)]);
      remove_piece(move_piece(m), move_from(m));
      add_piece(Rook + side_mask(m), rook_castles_to[move_to(m)]);
      add_piece(move_piece(m), move_to(m));
    }

    if ((move_piece(m) & 7) == King)
      king_square[move_side(m)] = move_to(m);
  }

  void unmake_move(const uint32_t m) {
    if (!is_castle_move(m))
    {
      if (move_type(m) & PROMOTION)
        remove_piece(move_promoted(m), move_to(m));
      else
        remove_piece(move_piece(m), move_to(m));

      if (is_ep_capture(m))
      {
        if (move_piece(m) < 8)
          add_piece(moveCaptured(m), move_to(m) - 8);
        else
          add_piece(moveCaptured(m), move_to(m) + 8);
      } else if (is_capture(m))
        add_piece(moveCaptured(m), move_to(m));

      add_piece(move_piece(m), move_from(m));
    } else
    {
      remove_piece(move_piece(m), move_to(m));
      remove_piece(Rook + side_mask(m), rook_castles_to[move_to(m)]);
      add_piece(move_piece(m), move_from(m));
      add_piece(Rook + side_mask(m), rook_castles_from[move_to(m)]);
    }

    if ((move_piece(m) & 7) == King)
      king_square[move_side(m)] = move_from(m);
  }

  [[nodiscard]]
  int get_piece(const uint64_t sq) const { return board[sq]; }

  [[nodiscard]]
  int get_piece_type(const uint64_t sq) const { return board[sq] & 7; }

  [[nodiscard]]
  uint64_t get_pinned_pieces(const int side, const uint64_t sq) {
    uint64_t pinned_pieces           = 0;
    const auto opp           = side ^ 1;
    auto pinners       = xray_bishop_attacks(occupied, occupied_by_side[side], sq) & (piece[Bishop + (opp << 3)] | piece[Queen | opp << 3]);

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

  static uint64_t xray_rook_attacks(const uint64_t &occ, uint64_t blockers, const uint64_t sq) {
    const auto attacks = rookAttacks(sq, occ);
    blockers &= attacks;
    return attacks ^ rookAttacks(sq, occ ^ blockers);
  }

  static uint64_t xray_bishop_attacks(const uint64_t &occ, uint64_t blockers, const uint64_t sq) {
    const auto attacks = bishopAttacks(sq, occ);
    blockers &= attacks;
    return attacks ^ bishopAttacks(sq, occ ^ blockers);
  }

  [[nodiscard]]
  uint64_t is_occupied(const uint64_t sq) const { return bb_square(sq) & occupied; }

  [[nodiscard]]
  bool is_attacked(const uint64_t sq, const int side) const {
    return is_attacked_by_slider(sq, side) || is_attacked_by_knight(sq, side) || is_attacked_by_pawn(sq, side) || is_attacked_by_king(sq, side);
  }

  [[nodiscard]]
  uint64_t piece_attacks(const int pc, const uint64_t sq) const {
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
  bool is_attacked_by_slider(const uint64_t sq, const int side) const {
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
  bool is_attacked_by_knight(const uint64_t sq, const int side) const { return (piece[Knight + (side << 3)] & knight_attacks[sq]) != 0; }

  [[nodiscard]]
  bool is_attacked_by_pawn(const uint64_t sq, const int side) const { return (piece[Pawn | side << 3] & pawn_captures[sq | (side ^ 1) << 6]) != 0; }

  [[nodiscard]]
  bool is_attacked_by_king(const uint64_t sq, const int side) const { return (piece[King | side << 3] & king_attacks[sq]) != 0; }

  void print() const {
    constexpr std::string_view piece_letter = "PNBRQK. pnbrqk. ";
    printf("\n");

    for (auto rank = 7; rank >= 0; rank--)
    {
      printf("%d  ", rank + 1);

      for (auto file = 0; file <= 7; file++)
      {
        const auto p_and_c = get_piece(rank * 8 + file);
        printf("%c ", piece_letter[p_and_c]);
      }
      printf("\n");
    }
    printf("   a b c d e f g h\n");
  }

  [[nodiscard]]
  const uint64_t &pieces(const int p, const int side) const { return piece[p | side << 3]; }

  [[nodiscard]]
  const uint64_t &pawns(const int side) const { return piece[Pawn | side << 3]; }

  [[nodiscard]]
  const uint64_t &knights(const int side) const { return piece[Knight | side << 3]; }

  [[nodiscard]]
  const uint64_t &bishops(const int side) const { return piece[Bishop | side << 3]; }

  [[nodiscard]]
  const uint64_t &rooks(const int side) const { return piece[Rook | side << 3]; }

  [[nodiscard]]
  const uint64_t &queens(const int side) const { return piece[Queen | side << 3]; }

  [[nodiscard]]
  const uint64_t &king(const int side) const { return piece[King | side << 3]; }

  std::array<uint64_t, 2 << 3> piece{};
  std::array<uint64_t, 2> occupied_by_side{};
  std::array<int, 64> board{};
  std::array<uint64_t, 2> king_square{};
  uint64_t queen_attacks{};
  uint64_t occupied{};

  [[nodiscard]]
  bool is_pawn_passed(const uint64_t sq, const int side) const { return (passed_pawn_front_span[side][sq] & pawns(side ^ 1)) == 0; }

  [[nodiscard]]
  bool is_piece_on_square(const int p, const uint64_t sq, const int side) { return (bb_square(sq) & piece[p + (side << 3)]) != 0; }

  [[nodiscard]]
  bool is_piece_on_file(const int p, const uint64_t sq, const int side) const { return (bb_file(sq) & piece[p + (side << 3)]) != 0; }

  [[nodiscard]]
  bool is_pawn_isolated(const uint64_t sq, const int side) const {
    const auto &bb      = bb_square(sq);
    const auto neighbour_files = north_fill(south_fill(west_one(bb) | east_one(bb)));
    return (pawns(side) & neighbour_files) == 0;
  }

  [[nodiscard]]
  bool is_pawn_behind(const uint64_t sq, const int side) const {
    const auto &bbsq = bb_square(sq);
    return (pawns(side) & pawn_fill[side ^ 1](west_one(bbsq) | east_one(bbsq))) == 0;
  }
};