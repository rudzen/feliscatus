#pragma once

#include <array>
#include <cstdint>
#include "move.h"
#include "board.h"
#include "bitboard.h"

struct MoveData {
  uint32_t move;
  int score;
};

struct MoveSorter {
  virtual ~MoveSorter() = default;
  virtual void sort_move(MoveData &move_data) = 0;
};

static constexpr int LEGALMOVES     = 1;
static constexpr int STAGES         = 2;
static constexpr int QUEENPROMOTION = 4;

class Moves {
public:
  void generate_moves(MoveSorter *sorter = nullptr, const uint32_t ttMove = 0, const int flags = 0) {
    reset(sorter, ttMove, flags);
    max_stage = 3;

    if ((this->move_flags & STAGES) == 0)
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

  void generate_moves(const int piece, const Bitboard to_squares) {
    reset(nullptr, 0, 0);

    for (auto bb = board->piece[piece]; bb; reset_lsb(bb))
    {
      const auto from = lsb(bb);
      add_moves(piece, from, board->piece_attacks(piece, from) & to_squares);
    }
  }

  void generate_pawn_moves(const bool capture, const Bitboard to_squares) {
    reset(nullptr, 0, 0);

    if (capture)
      add_pawn_capture_moves(to_squares);
    else
      add_pawn_quiet_moves(to_squares);
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
      return nullptr;

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
    if ((bb_piece[move_piece(m)] & move_from(m)) == 0)
      return false;

    const auto to = move_to(m);

    if (is_capture(m))
    {
      if ((occupied_by_side[~move_side(m)] & to) == 0)
        return false;

      if ((bb_piece[moveCaptured(m)] & to) == 0)
        return false;
    } else if (occupied & to)
      return false;

    if (const auto piece = move_piece(m) & 7; piece == Bishop || piece == Rook || piece == Queen)
    {
      if (between_bb[move_from(m)][to] & occupied)
        return false;
    }
    return true;
  }

  std::array<MoveData, 256> move_list{};

private:
  void reset(MoveSorter *sorter, const uint32_t move, const int flags) {
    move_sorter = sorter;
    transp_move = move;
    move_flags  = flags;

    if (move)
    {
      if (is_castle_move(this->transp_move) || is_ep_capture(this->transp_move))
      {
        // needed because isPseudoLegal() is not complete yet.
        this->transp_move = 0;
        this->move_flags &= ~STAGES;
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
    add_moves(occupied_by_side[~side_to_move]);
    const auto &pawns = board->pawns(side_to_move);
    add_pawn_moves(pawn_push(side_to_move, pawns & rank_7[side_to_move]) & ~occupied, pawn_push(side_to_move), QUIET);
    add_pawn_moves(pawn_west_attacks[side_to_move](pawns) & occupied_by_side[~side_to_move], pawn_west_attack_dist[side_to_move], CAPTURE);
    add_pawn_moves(pawn_east_attacks[side_to_move](pawns) & occupied_by_side[~side_to_move], pawn_east_attack_dist[side_to_move], CAPTURE);
    if (en_passant_square != no_square)
    {
      add_pawn_moves(pawn_west_attacks[side_to_move](pawns) & en_passant_square, pawn_west_attack_dist[side_to_move], EPCAPTURE);
      add_pawn_moves(pawn_east_attacks[side_to_move](pawns) & en_passant_square, pawn_east_attack_dist[side_to_move], EPCAPTURE);
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
    const auto pushed = pawn_push(side_to_move, board->pawns(side_to_move) & ~rank_7[side_to_move]) & ~occupied;
    add_pawn_moves(pushed, pawn_push(side_to_move), QUIET);
    add_pawn_moves(pawn_push(side_to_move, pushed & rank_3[side_to_move]) & ~occupied, pawn_push(side_to_move) * 2, DOUBLEPUSH);
    add_moves(~occupied);
    stage++;
  }

  void add_move(const int piece, const Square from, const Square to, const uint32_t type, const int promoted = 0) {
    uint32_t move;
    int captured;

    if (type & CAPTURE)
      captured = board->get_piece(to);
    else if (type & EPCAPTURE)
      captured = Pawn | ((~side_to_move) << 3);
    else
      captured = 0;

    init_move(move, piece, captured, from, to, type, promoted);

    if (transp_move == move)
      return;

    if ((move_flags & LEGALMOVES) && !is_legal(move, piece, from, type))
      return;

    auto &move_data = move_list[number_moves++];
    move_data.move  = move;

    if (move_sorter)
      move_sorter->sort_move(move_data);
    else
      move_data.score = 0;
  }

  void add_moves(const Bitboard to_squares) {
    Bitboard bb;
    const auto offset = side_to_move << 3;
    Square from;

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

  void add_moves(const int piece, const Square from, const Bitboard attacks) {
    for (auto bb = attacks; bb; reset_lsb(bb))
    {
      const auto to = lsb(bb);
      add_move(piece | (side_to_move << 3), from, to, board->get_piece(to) == NoPiece ? QUIET : CAPTURE);
    }
  }

  void add_pawn_quiet_moves(const Bitboard to_squares) {
    const auto pushed = pawn_push(side_to_move, board->pawns(side_to_move)) & ~occupied;
    add_pawn_moves(pushed & to_squares, pawn_push(side_to_move), QUIET);
    add_pawn_moves(pawn_push(side_to_move, pushed & rank_3[side_to_move]) & ~occupied & to_squares, pawn_push(side_to_move) * 2, DOUBLEPUSH);
  }

  void add_pawn_capture_moves(const Bitboard to_squares) {
    const auto &pawns = board->pawns(side_to_move);
    add_pawn_moves(pawn_west_attacks[side_to_move](pawns) & occupied_by_side[~side_to_move] & to_squares, pawn_west_attack_dist[side_to_move], CAPTURE);
    add_pawn_moves(pawn_east_attacks[side_to_move](pawns) & occupied_by_side[~side_to_move] & to_squares, pawn_east_attack_dist[side_to_move], CAPTURE);
    if (en_passant_square != no_square)
    {
      add_pawn_moves(pawn_west_attacks[side_to_move](pawns) & to_squares & en_passant_square, pawn_west_attack_dist[side_to_move], EPCAPTURE);
      add_pawn_moves(pawn_east_attacks[side_to_move](pawns) & to_squares & en_passant_square, pawn_east_attack_dist[side_to_move], EPCAPTURE);
    }
  }

  void add_pawn_moves(const Bitboard to_squares, const Direction dist, const uint32_t type) {
    for (auto bb = to_squares; bb; reset_lsb(bb))
    {
      const auto to = lsb(bb);
      const auto from = to - dist;
      const auto rr = relative_rank(side_to_move, to);

      if (rr == RANK_8)
      {
        if (move_flags & QUEENPROMOTION)
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

  void add_castle_move(const Square from, const Square to) { add_move(King | (side_to_move << 3), from, to, CASTLE); }

  [[nodiscard]]
  bool gives_check(const uint32_t m) const {
    board->make_move(m);
    const bool is_attacked = board->is_attacked(board->king_square[~side_to_move], side_to_move);
    board->unmake_move(m);
    return is_attacked;
  }

  [[nodiscard]]
  bool is_legal(const uint32_t m, const int piece, const Square from, const uint32_t type) const {
    if ((pinned & from) || in_check || (piece & 7) == King || (type & EPCAPTURE))
    {
      board->make_move(m);

      if (board->is_attacked(board->king_square[side_to_move], ~side_to_move))
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
  bool is_castle_allowed(const Square to, const Color stm) const {
    // A bit complicated because of Chess960. See http://en.wikipedia.org/wiki/Chess960
    // The following comments were taken from that source.

    // Check that the smallest back rank interval containing the king, the castling rook, and their
    // destination squares, contains no pieces other than the king and castling rook.

    const auto rook_to          = rook_castles_to[to];
    const auto rook_from        = rook_castles_from[to];
    const auto king_square      = board->king_square[stm];
    const auto bb_castle_pieces = bit(rook_from, king_square);
    const auto bb_castle_span   = bb_castle_pieces | between_bb[king_square][rook_from] | between_bb[rook_from][rook_to] | bit(rook_to, to);

    if ((bb_castle_span & occupied) != bb_castle_pieces)
      return false;

    // Check that no square between the king's initial and final squares (including the initial and final
    // squares) may be under attack by an enemy piece. (Initial square was already checked a this point.)

    for (auto bb = between_bb[king_square][to] | to; bb; reset_lsb(bb))
    {
      if (board->is_attacked(lsb(bb), ~stm))
        return false;
    }
    return true;
  }

public:
  Color side_to_move{};
  int castle_rights{};
  bool in_check{};
  Square en_passant_square{};
  Board *board{};

private:
  Bitboard *bb_piece{};
  Bitboard *occupied_by_side{};
  Bitboard occupied{};
  int iteration{};
  int stage{};
  int max_stage{};
  int number_moves{};
  Bitboard pinned{};
  MoveSorter *move_sorter{};
  uint32_t transp_move{};
  int move_flags{};
};