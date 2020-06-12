#pragma once

#include <cstdint>
#include "move.h"
#include "board.h"
#include "bitboard.h"

struct MoveData {
  uint32_t move;
  int score;
};

class MoveSorter {
public:
  virtual void sort_move(MoveData &move_data) = 0;
};

static constexpr int LEGALMOVES     = 1;
static constexpr int STAGES         = 2;
static constexpr int QUEENPROMOTION = 4;

class Moves {
public:
  void generate_moves(MoveSorter *sorter = nullptr, const uint32_t transp_move = 0, const int flags = 0) {
    reset(sorter, transp_move, flags);
    max_stage = 3;

    if ((this->flags & STAGES) == 0)
    {
      generate_hash_move();
      generate_captures_and_promotions();
      generate_quiet_moves();
    }
  }

  void generate_captures_and_promotions(MoveSorter *sorter) {
    reset(sorter, 0, QUEENPROMOTION | STAGES);
    max_stage = 2;
    stage     = 1;
  }

  void generate_moves(const int piece, const uint64_t &to_squares) {
    reset(0, 0, 0);

    for (auto bb = board->piece[piece]; bb; reset_lsb(bb))
    {
      const uint64_t from = lsb(bb);
      add_moves(piece, from, board->piece_attacks(piece, from) & to_squares);
    }
  }

  void generate_pawn_moves(const bool capture, const uint64_t &to_squares) {
    reset(nullptr, 0, 0);

    if (capture)
    {
      add_pawn_capture_moves(to_squares);
    } else
    { add_pawn_quiet_moves(to_squares); }
  }

  [[nodiscard]]
  MoveData *next_move() {
    while (iteration == number_moves && stage < max_stage)
    {
      switch (stage)
      {
      case 0:
        generate_hash_move();
        break;

      case 1:
        generate_captures_and_promotions();
        break;

      case 2:
        generate_quiet_moves();
        break;

      default:// error
        return nullptr;
      }
    }

    if (iteration == number_moves)
    {
      return nullptr;
    }

    do
    {
      auto best_idx   = iteration;
      auto best_score = move_list[best_idx].score;

      for (auto i = best_idx + 1; i < number_moves; i++)
      {
        if (move_list[i].score > best_score)
        {
          best_score = move_list[i].score;
          best_idx   = i;
        }
      }

      if (max_stage > 2 && stage == 2 && move_list[best_idx].score < 0)
      {
        generate_quiet_moves();
        continue;
      }
      const auto tmp       = move_list[iteration];
      move_list[iteration] = move_list[best_idx];
      move_list[best_idx]  = tmp;
      return &move_list[iteration++];
    } while (true);
  }

  [[nodiscard]]
  int move_count() const { return number_moves; }

  void goto_move(const int pos) { iteration = pos; }

  [[nodiscard]]
  bool is_pseudo_legal(const uint32_t m) const {
    // TO DO en passant moves and castle moves
    if ((bb_piece[move_piece(m)] & bb_square(move_from(m))) == 0)
      return false;

    if (is_capture(m))
    {
      const auto &bb_to = bb_square(move_to(m));

      if ((occupied_by_side[move_side(m) ^ 1] & bb_to) == 0)
        return false;

      if ((bb_piece[moveCaptured(m)] & bb_to) == 0)
        return false;
    } else if (occupied & bb_square(move_to(m)))
      return false;

    const auto piece = move_piece(m) & 7;

    if (piece == Bishop || piece == Rook || piece == Queen)
    {
      if (between_bb[move_from(m)][move_to(m)] & occupied)
        return false;
    }
    return true;
  }

  MoveData move_list[256];

private:
  void reset(MoveSorter *sorter, const uint32_t move, const int flags) {
    this->sorter      = sorter;
    this->transp_move = move;
    this->flags       = flags;

    if (move)
    {
      if (is_castle_move(this->transp_move) || is_ep_capture(this->transp_move))
      {
        // needed because isPseudoLegal() is not complete yet.
        this->transp_move = 0;
        this->flags &= ~STAGES;
      }
    }

    iteration    = 0;
    number_moves = 0;
    stage        = 0;

    if (flags & LEGALMOVES)
      pinned = board->get_pinned_pieces(side_to_move, board->king_square[side_to_move]);

    occupied         = board->occupied;
    occupied_by_side = board->occupied_by_side.data();
    bb_piece         = board->piece.data();
  }

  void generate_hash_move() {
    if (transp_move && is_pseudo_legal(transp_move))
    {
      move_list[number_moves].score  = 890010;
      move_list[number_moves++].move = transp_move;
    }
    stage++;
  }

  void generate_captures_and_promotions() {
    add_moves(occupied_by_side[side_to_move ^ 1]);
    const auto &pawns = board->pawns(side_to_move);
    add_pawn_moves(pawn_push[side_to_move](pawns & rank_7[side_to_move]) & ~occupied, pawn_push_dist, QUIET);
    add_pawn_moves(pawn_west_attacks[side_to_move](pawns) & occupied_by_side[side_to_move ^ 1], pawn_west_attack_dist, CAPTURE);
    add_pawn_moves(pawn_east_attacks[side_to_move](pawns) & occupied_by_side[side_to_move ^ 1], pawn_east_attack_dist, CAPTURE);
    if (en_passant_square != no_square)
    {
      add_pawn_moves(pawn_west_attacks[side_to_move](pawns) & bb_square(en_passant_square), pawn_west_attack_dist, EPCAPTURE);
      add_pawn_moves(pawn_east_attacks[side_to_move](pawns) & bb_square(en_passant_square), pawn_east_attack_dist, EPCAPTURE);
    }
    stage++;
  }

  void generate_quiet_moves() {
    if (!in_check)
    {
      if (can_castle_short())
        add_castle_move(oo_king_from[side_to_move], oo_king_to[side_to_move]);

      if (can_castle_long())
        add_castle_move(ooo_king_from[side_to_move], ooo_king_to[side_to_move]);
    }
    const auto pushed = pawn_push[side_to_move](board->pawns(side_to_move) & ~rank_7[side_to_move]) & ~occupied;
    add_pawn_moves(pushed, pawn_push_dist, QUIET);
    add_pawn_moves(pawn_push[side_to_move](pushed & rank_3[side_to_move]) & ~occupied, pawn_double_push_dist, DOUBLEPUSH);
    add_moves(~occupied);
    stage++;
  }

  void add_move(const int piece, const uint64_t from, const uint64_t to, const uint32_t type, const int promoted = 0) {
    uint32_t move;
    int captured;

    if (type & CAPTURE)
      captured = board->get_piece(to);
    else if (type & EPCAPTURE)
      captured = Pawn | ((side_to_move ^ 1) << 3);
    else
      captured = 0;

    init_move(move, piece, captured, from, to, type, promoted);

    if (transp_move == move)
      return;

    if ((flags & LEGALMOVES) && !is_legal(move, piece, from, type))
      return;

    auto &move_data = move_list[number_moves++];
    move_data.move  = move;

    if (sorter)
      sorter->sort_move(move_data);
    else
      move_data.score = 0;
  }

  void add_moves(const uint64_t &to_squares) {
    uint64_t bb;
    const auto offset = side_to_move << 3;
    uint64_t from;

    for (bb = bb_piece[Queen + offset]; bb; reset_lsb(bb))
    {
      from = lsb(bb);
      add_moves(Queen + offset, from, queenAttacks(from, board->occupied) & to_squares);
    }

    for (bb = bb_piece[Rook + offset]; bb; reset_lsb(bb))
    {
      from = lsb(bb);
      add_moves(Rook + offset, from, rookAttacks(from, board->occupied) & to_squares);
    }

    for (bb = bb_piece[Bishop + offset]; bb; reset_lsb(bb))
    {
      from = lsb(bb);
      add_moves(Bishop + offset, from, bishopAttacks(from, board->occupied) & to_squares);
    }

    for (bb = bb_piece[Knight + offset]; bb; reset_lsb(bb))
    {
      from = lsb(bb);
      add_moves(Knight + offset, from, knightAttacks(from) & to_squares);
    }

    for (bb = bb_piece[King + offset]; bb; reset_lsb(bb))
    {
      from = lsb(bb);
      add_moves(King + offset, from, kingAttacks(from) & to_squares);
    }
  }

  void add_moves(const int piece, const uint64_t from, const uint64_t &attacks) {
    for (auto bb = attacks; bb; reset_lsb(bb))
    {
      const uint64_t to = lsb(bb);
      add_move(piece | (side_to_move << 3), from, to, board->get_piece(to) == NoPiece ? QUIET : CAPTURE);
    }
  }

  void add_pawn_quiet_moves(const uint64_t &to_squares) {
    const auto pushed = pawn_push[side_to_move](board->pawns(side_to_move)) & ~occupied;
    add_pawn_moves(pushed & to_squares, pawn_push_dist, QUIET);
    add_pawn_moves(pawn_push[side_to_move](pushed & rank_3[side_to_move]) & ~occupied & to_squares, pawn_double_push_dist, DOUBLEPUSH);
  }

  void add_pawn_capture_moves(const uint64_t &to_squares) {
    const auto &pawns = board->pawns(side_to_move);
    add_pawn_moves(pawn_west_attacks[side_to_move](pawns) & occupied_by_side[side_to_move ^ 1] & to_squares, pawn_west_attack_dist, CAPTURE);
    add_pawn_moves(pawn_east_attacks[side_to_move](pawns) & occupied_by_side[side_to_move ^ 1] & to_squares, pawn_east_attack_dist, CAPTURE);
    if (en_passant_square != no_square)
    {
      add_pawn_moves(pawn_west_attacks[side_to_move](pawns) & bb_square(en_passant_square) & to_squares, pawn_west_attack_dist, EPCAPTURE);
      add_pawn_moves(pawn_east_attacks[side_to_move](pawns) & bb_square(en_passant_square) & to_squares, pawn_east_attack_dist, EPCAPTURE);
    }
  }

  void add_pawn_moves(const uint64_t &to_squares, const std::array<int, 2> &dist, const uint32_t type) {
    for (auto bb = to_squares; bb; reset_lsb(bb))
    {
      const uint64_t to = lsb(bb);
      const auto from   = to - dist[side_to_move];

      if (rank_of(to) == 0 || rank_of(to) == 7)
      {
        if (flags & QUEENPROMOTION)
        {
          add_move(Pawn | (side_to_move << 3), from, to, type | PROMOTION, Queen | (side_to_move << 3));
          return;
        }

        for (auto promoted = Queen; promoted >= Knight; promoted--)
          add_move(Pawn | (side_to_move << 3), from, to, type | PROMOTION, promoted | (side_to_move << 3));
      } else
        add_move(Pawn | (side_to_move << 3), from, to, type);
    }
  }

  void add_castle_move(const uint64_t from, const uint64_t to) { add_move(King | (side_to_move << 3), from, to, CASTLE); }

  [[nodiscard]]
  bool gives_check(const uint32_t m) const {
    board->make_move(m);
    const bool is_attacked = board->is_attacked(board->king_square[side_to_move ^ 1], side_to_move);
    board->unmake_move(m);
    return is_attacked;
  }

  [[nodiscard]]
  bool is_legal(const uint32_t m, const int piece, const uint64_t from, const uint32_t type) const {
    if ((pinned & bb_square(from)) || in_check || (piece & 7) == King || (type & EPCAPTURE))
    {
      board->make_move(m);

      if (board->is_attacked(board->king_square[side_to_move], side_to_move ^ 1))
      {
        board->unmake_move(m);
        return false;
      }
      board->unmake_move(m);
    }
    return true;
  }

  [[nodiscard]]
  bool can_castle_short() const { return (castle_rights & oo_allowed_mask[side_to_move]) && is_castle_allowed(oo_king_to[side_to_move], side_to_move); }

  [[nodiscard]]
  bool can_castle_long() const { return (castle_rights & ooo_allowed_mask[side_to_move]) && is_castle_allowed(ooo_king_to[side_to_move], side_to_move); }

  [[nodiscard]]
  bool is_castle_allowed(uint64_t to, const int side_to_move) const {
    // A bit complicated because of Chess960. See http://en.wikipedia.org/wiki/Chess960
    // The following comments were taken from that source.

    // Check that the smallest back rank interval containing the king, the castling rook, and their
    // destination squares, contains no pieces other than the king and castling rook.

    const uint64_t rook_to   = rook_castles_to[to];
    const uint64_t rook_from = rook_castles_from[to];
    const auto king_square   = board->king_square[side_to_move];

    const auto bb_castle_pieces = bb_square(rook_from) | bb_square(king_square);

    const auto bb_castle_span = bb_castle_pieces | bb_square(rook_to) | bb_square(to) | between_bb[king_square][rook_from] | between_bb[rook_from][rook_to];

    if ((bb_castle_span & occupied) != bb_castle_pieces)
      return false;

    // Check that no square between the king's initial and final squares (including the initial and final
    // squares) may be under attack by an enemy piece. (Initial square was already checked a this point.)

    for (auto bb = between_bb[king_square][to] | bb_square(to); bb; reset_lsb(bb))
    {
      if (board->is_attacked(lsb(bb), side_to_move ^ 1))
        return false;
    }
    return true;
  }

public:
  int side_to_move;
  int castle_rights;
  bool in_check;
  Square en_passant_square;
  Board *board;

private:
  Bitboard *bb_piece{};
  Bitboard *occupied_by_side{};
  Bitboard occupied{};
  int iteration{};
  int stage{};
  int max_stage{};
  int number_moves{};
  Bitboard pinned{};
  MoveSorter *sorter{};
  uint32_t transp_move{};
  int flags{};
};