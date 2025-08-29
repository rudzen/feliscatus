// Copyright (C) 2008-2016 Gunnar Harms (Bobcat author)
// Copyright (C) 2017      FireFather (Tomcat author)
// Copyright (C) 2020-2025 Rudy Alex Kohn
// See end of file for extended copyright information.

#include <string>
#include <cctype>
#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <felis/board.hpp>
#include <felis/bitboard.hpp>
#include <felis/util.hpp>
#include <felis/transpositional.hpp>
#include <felis/miscellaneous.hpp>
#include <felis/prng.hpp>
#include <felis/moves.hpp>
#include <felis/zobrist.hpp>

namespace
{

/// indexed by the position of the king
std::array<Square, SQ_NB> rook_castles_to{NO_SQ};
std::array<Square, SQ_NB> rook_castles_from{NO_SQ};

constexpr i32 max_log_file_size = 1048576 * 5;
constexpr i32 max_log_files     = 3;

const std::shared_ptr<spdlog::logger> logger =
  spdlog::rotating_logger_mt("castleling_logger", "logs/castleling.txt", max_log_file_size, max_log_files);

[[nodiscard]]
Square ep_square(std::string_view s, const Color stm)
{
  [[likely]]
  if (s.empty() || s.length() == 1 || s.front() == '-')
    return NO_SQ;

  const auto first = s.front();

  [[unlikely]]
  if (!util::inBetween<'a', 'h'>(first))
    return NO_SQ;

  s.remove_prefix(1);

  const auto target_row = stm == WHITE ? '6' : '3';

  return s.front() != target_row ? NO_SQ : static_cast<Square>(first - 'a' + (s.front() - '1') * 8);
}

template<CastlingRight Side>
[[nodiscard]]
Square find_rook_square(const Color us, const Board *b)
{
  const Piece rook = make_piece(ROOK, us);

  const auto Relative = [&us](const Square s) {
    return relative_square(us, s);
  };

  const auto is_rook_on_square = [&b, &rook](const Square sq) {
    return b->piece(sq) == rook;
  };

  const auto filter = std::views::transform(Relative) | std::views::filter(is_rook_on_square);

  if constexpr (Side == KING_SIDE)
  {
    auto res = CastlelingSquaresKing | filter;
    return *std::ranges::begin(res);
  } else
  {
    auto res = CastlelingSquaresQueen | filter;
    return *std::ranges::begin(res);
  }
}

void update_key(Position *pos, const Move m)
{
  Key pawn_key = pos->pawn_structure_key;
  Key key      = pos->key ^ pawn_key;

  pawn_key ^= zobrist.side();
  const Position *prev = pos->previous;   // (pos - 1);

  [[unlikely]]
  if (prev->en_passant_square != NO_SQ)
    key ^= zobrist.ep(file_of(prev->en_passant_square));

  [[unlikely]]
  if (pos->en_passant_square != NO_SQ)
    key ^= zobrist.ep(file_of(pos->en_passant_square));

  [[unlikely]]
  if (!m)
  {
    key ^= pawn_key;
    pos->key                = key;
    pos->pawn_structure_key = pawn_key;
    return;
  }

  const Piece piece  = move_piece(m);
  const bool is_pawn = type_of(piece) == PAWN;
  const Square from  = move_from(m);
  const Square to    = move_to(m);
  const MoveType mt  = type_of(m);

  // from and to for moving piece
  [[likely]]
  if (is_pawn)
    pawn_key ^= zobrist.pst(piece, from);
  else
    key ^= zobrist.pst(piece, from);

  [[unlikely]]
  if (mt & PROMOTION)
    key ^= zobrist.pst(move_promoted(m), to);
  else
  {
    [[likely]]
    if (is_pawn)
      pawn_key ^= zobrist.pst(piece, to);
    else
      key ^= zobrist.pst(piece, to);
  }

  // remove captured piece
  [[unlikely]]
  if (mt & EPCAPTURE)
    pawn_key ^= zobrist.pst(move_captured(m), to + pawn_push(pos->side_to_move));
  else if (mt & CAPTURE)
  {
    [[likely]]
    if (is_pawn)
      pawn_key ^= zobrist.pst(move_captured(m), to);
    else
      key ^= zobrist.pst(move_captured(m), to);
  }

  // castling rights
  [[unlikely]]
  if (prev->castle_rights != pos->castle_rights)
    key ^= zobrist.castle(prev->castle_rights) ^ zobrist.castle(pos->castle_rights);

  // rook move in castle
  [[unlikely]]
  if (mt & CASTLE)
  {
    const Piece rook = make_piece(ROOK, move_side(m));
    key ^= zobrist.pst(rook, rook_castles_from[to]) ^ zobrist.pst(rook, rook_castles_to[to]);
  }
  key ^= pawn_key;
  pos->key                = key;
  pos->pawn_structure_key = pawn_key;
}

}   // namespace

Board::Board() : position_list({})
{
  pos = position_list.data();
}

void Board::init()
{
  for (const Color side : Colors)
  {
    const auto rank1                            = relative_rank(side, RANK_1);
    rook_castles_to[make_square(FILE_G, rank1)] = make_square(FILE_F, rank1);
    rook_castles_to[make_square(FILE_C, rank1)] = make_square(FILE_D, rank1);
  }
}

void Board::clear()
{
  occupied_by_side.fill(ZeroBB);
  occupied_by_type.fill(ZeroBB);
  board.fill(NO_PIECE);
  max_ply = plies = search_depth = 0;
  oo_king_from.fill(NO_SQ);
  ooo_king_from.fill(NO_SQ);
  castling_path.fill(ZeroBB);
}

void Board::perform_move(const Move m)
{
  const Square from = move_from(m);
  const Square to   = move_to(m);
  const MoveType mt = type_of(m);
  Piece pc          = move_piece(m);

  [[unlikely]]
  if (mt & CASTLE)
  {
    const Piece rook = make_piece(ROOK, move_side(m));
    remove_piece(rook_castles_from[to]);
    remove_piece(from);
    add_piece(rook, rook_castles_to[to]);
    add_piece(pc, to);
  } else
  {
    remove_piece(from);

    [[unlikely]]
    if (mt & EPCAPTURE)
    {
      const Direction direction = pawn_push(color_of(pc));
      remove_piece(to - direction);
    } else if (mt & CAPTURE)
      remove_piece(to);

    [[unlikely]]
    if (mt & PROMOTION)
      pc = move_promoted(m);

    add_piece(pc, to);
  }
}

void Board::unperform_move(const Move m)
{
  const Square from = move_from(m);
  const Square to   = move_to(m);
  const Piece pc    = move_piece(m);
  const MoveType mt = type_of(m);

  [[unlikely]]
  if (mt & CASTLE)
  {
    const Piece rook = make_piece(ROOK, move_side(m));
    remove_piece(to);
    remove_piece(rook_castles_to[to]);
    add_piece(pc, from);
    add_piece(rook, rook_castles_from[to]);
  } else
  {
    remove_piece(to);

    [[unlikely]]
    if (mt & EPCAPTURE)
    {
      const Direction direction = pawn_push(color_of(pc));
      add_piece(move_captured(m), to - direction);
    } else if (mt & CAPTURE)
      add_piece(move_captured(m), to);

    add_piece(pc, from);
  }
}

Bitboard Board::pinned_pieces(const Color c, const Square s) const
{
  const Color them           = ~c;
  const Bitboard all_pieces  = pieces();
  const Bitboard side_pieces = pieces(c);
  Bitboard pinners           = xray_attacks<BISHOP>(all_pieces, side_pieces, s) & pieces(BISHOP, QUEEN, them);
  Bitboard pinned_pieces     = ZeroBB;

  while (pinners)
    pinned_pieces |= between(pop_lsb(&pinners), s) & side_pieces;

  pinners = xray_attacks<ROOK>(all_pieces, side_pieces, s) & pieces(ROOK, QUEEN, them);

  while (pinners)
    pinned_pieces |= between(pop_lsb(&pinners), s) & side_pieces;

  return pinned_pieces;
}

bool Board::is_attacked_by_slider(const Square s, const Color c) const
{
  const Bitboard all_pieces = pieces();
  const Bitboard r_attacks  = piece_attacks_bb<ROOK>(s, all_pieces);

  if (pieces(ROOK, c) & r_attacks)
    return true;

  const Bitboard b_attacks = piece_attacks_bb<BISHOP>(s, all_pieces);

  if (pieces(BISHOP, c) & b_attacks)
    return true;

  return pieces(QUEEN, c) & (b_attacks | r_attacks);
}

bool Board::is_pseudo_legal(const Move m) const
{
  // TODO : castleling & en passant moves

  assert(is_ok(m));

  const Square from = move_from(m);
  const Piece pc    = move_piece(m);

  [[unlikely]]
  if ((pieces(pc) & from) == 0)
    return false;

  const Square to      = move_to(m);
  const Color move_stm = move_side(m);

  [[unlikely]]
  if (move_stm != side_to_move())
    return false;

  if (is_capture(m))
  {
    if ((pieces(~move_stm) & to) == 0)
      return false;

    if ((pieces(move_captured(m)) & to) == 0)
      return false;
  }
  // } else if (is_castle_move(m))
  //    return !b->is_attacked(b->king_sq(side_to_move), side_to_move) && !in_check && ((from < to &&
  //    can_castle_short()) || (from > to && can_castle_long()));
  else if (pieces() & to)
    return false;

  const PieceType pt = type_of(move_piece(m));

  return !util::inBetween<QUEEN, BISHOP>(pt) || !(between(from, to) & pieces());
}

void Board::print() const
{
  fmt::memory_buffer s;
  auto inserter = std::back_inserter(s);

  fmt::format_to(inserter, "\n");

  for (const Rank rank : ReverseRanks)
  {
    fmt::format_to(inserter, "{}  ", rank + 1);

    for (const File file : Files)
    {
      const Square sq = make_square(file, rank);
      const Piece pc  = piece(sq);
      fmt::format_to(inserter, "{} ", piece_letter[pc]);
    }
    fmt::format_to(inserter, "\n");
  }

  fmt::print("{}   a b c d e f g h\n", fmt::to_string(s));
}

int Board::piece_count(const Color c, const PieceType pt) const
{
  return pos->material.count(c, pt);
}

bool Board::is_passed_pawn_move(const Move m) const
{
  return move_piece_type(m) == PAWN && is_pawn_passed(move_to(m), move_side(m));
}

bool Board::is_pawn_isolated(const Square s, const Color c) const
{
  const Bitboard f               = bb_file(file_of(s));
  const Bitboard neighbour_files = shift_bb<WEST>(f) | shift_bb<EAST>(f);
  return (pieces(PAWN, c) & neighbour_files) == 0;
}

bool Board::is_pawn_behind(const Square s, const Color c) const
{
  const Bitboard bbsq = bit(s);
  return (pieces(PAWN, c) & pawn_fill[~c](shift_bb<WEST>(bbsq) | shift_bb<EAST>(bbsq))) == 0;
}

bool Board::make_move(const Move m, const bool check_legal, const bool calculate_in_check)
{
  [[unlikely]]
  if (!m)
    return make_null_move();

  perform_move(m);

  const MoveType mt = type_of(m);

  if (check_legal && !(mt & CASTLE) && is_attacked(square<KING>(pos->side_to_move), ~pos->side_to_move))
  {
    unperform_move(m);
    return false;
  }

  const Square from = move_from(m);
  const Square to   = move_to(m);

  Position *const prev = pos++;

  pos->previous          = prev;
  pos->side_to_move      = ~prev->side_to_move;
  pos->material          = prev->material;
  pos->last_move         = m;
  pos->castle_rights     = prev->castle_rights;
  pos->null_moves_in_row = 0;

  pos->rule50 = mt & (CAPTURE | EPCAPTURE) || type_of(move_piece(m)) == PAWN ? 0 : prev->rule50 + 1;

  pos->en_passant_square  = type_of(m) & DOUBLEPUSH ? to + pawn_push(pos->side_to_move) : NO_SQ;
  pos->key                = prev->key;
  pos->pawn_structure_key = prev->pawn_structure_key;

  const Square ksq = square<KING>(pos->side_to_move);

  if (calculate_in_check)
    pos->in_check = is_attacked(ksq, ~pos->side_to_move);

  [[unlikely]]
  if (pos->in_check)
    pos->checkers = attackers_to(ksq);

  if (can_castle() && (castle_rights_mask[from] | castle_rights_mask[to]))
    pos->castle_rights &= ~(castle_rights_mask[from] | castle_rights_mask[to]);

  update_key(pos, m);

  prefetch(TT.findBucket(pos->key));

  pos->material.make_move(m);
  pos->pinned = pinned_pieces(pos->side_to_move, ksq);

  return true;
}

bool Board::make_move(const Move m, const bool check_legal)
{
  return make_move(m, check_legal, gives_check(m));
}

void Board::unmake_move()
{
  [[likely]]
  if (pos->last_move)
    unperform_move(pos->last_move);
  pos--;
}

bool Board::make_null_move()
{
  Position *const prev    = pos++;
  pos->previous           = prev;
  pos->side_to_move       = ~prev->side_to_move;
  pos->material           = prev->material;
  pos->last_move          = MOVE_NONE;
  pos->in_check           = false;
  pos->castle_rights      = prev->castle_rights;
  pos->null_moves_in_row  = prev->null_moves_in_row + 1;
  pos->rule50             = 0;
  pos->en_passant_square  = NO_SQ;
  pos->key                = prev->key;
  pos->pawn_structure_key = prev->pawn_structure_key;
  update_key(pos, MOVE_NONE);
  return true;
}

Key Board::calculate_key() const
{
  Key key = zobrist.zero();

  for (const PieceType pt : PieceTypes)
  {
    for (const Color c : Colors)
    {
      auto bb = pieces(pt, c);
      while (bb)
      {
        const Square sq = pop_lsb(&bb);
        const Piece pc  = piece(sq);
        key ^= zobrist.pst(pc, sq);
      }
    }
  }

  key ^= zobrist.castle(pos->castle_rights);

  [[unlikely]]
  if (pos->en_passant_square != NO_SQ)
    key ^= zobrist.ep(file_of(pos->en_passant_square));

  if (pos->side_to_move == BLACK)
    key ^= zobrist.side();

  return key;
}

bool Board::is_repetition() const
{
  i32 num_moves  = pos->rule50;
  Position *prev = pos;

  while ((num_moves = num_moves - 2) >= 0 && prev - position_list.data() > 1)
  {
    prev -= 2;

    [[unlikely]]
    if (prev->key == pos->key)
      return true;
  }
  return false;
}

i64 Board::half_move_count() const
{
  return pos->rule50;
  // TODO : fix implementation defined behaviour
  // return pos - position_list.data();
}

void Board::new_game(thread *t)
{
  set_fen(start_position, t);
}

void Board::set_fen(std::string_view fen, thread *t)
{
  pos = position_list.data();
  pos->clear();

  clear();

  constexpr char splitter = ' ';

  Square sq = A8;

  // indicates where in the fen the last space was located
  std::size_t space = 0;

  // updates the fen and the current view to the next part of the fen
  const auto update_current = [&fen, &space] {
    // remove already parsed section from fen
    fen.remove_prefix(space);

    // adjust space to next
    space = fen.find_first_of(splitter);

    // slice current view
    return fen.substr(0, space);
  };

  // the current view of the fen
  std::string_view current = update_current();

  for (const char token : current)
  {
    [[unlikely]]
    if (std::isdigit(token))
      sq += util::fromChar<int>(token) * EAST;
    else if (token == '/')
      sq += SOUTH * 2;
    else if (const std::size_t pc_idx = piece_index.find_first_of(tolower(token)); pc_idx != std::string_view::npos)
    {
      const Color c  = islower(token) ? BLACK : WHITE;
      const Piece pc = make_piece(static_cast<PieceType>(pc_idx), c);
      add_piece(pc, sq);
      ++sq;
    }
  }

  // Side to move
  space++;
  current           = update_current();
  pos->side_to_move = current.front() == 'w' ? WHITE : BLACK;

  // Castleling
  space++;
  current = update_current();
  setup_castling(current);

  // En-passant
  space++;
  current                = update_current();
  pos->en_passant_square = ep_square(current, pos->side_to_move);

  space++;
  current = update_current();

  pos->rule50 = util::toIntegral<int>(current);

  space++;
  current = update_current();

  plies = util::toIntegral<int>(current);
  plies = std::max(2 * (plies - 1), 0) + (pos->side_to_move == BLACK);

  update_position(pos);

  my_t = t;
}

std::string Board::fen() const
{
  fmt::memory_buffer buf;
  const auto s = std::back_inserter(buf);

  for (const Rank r : ReverseRanks)
  {
    u8 empty = 0;

    for (const auto f : Files)
    {
      const auto sq = make_square(f, r);
      const auto pc = piece(sq);

      [[unlikely]]
      if (pc != NO_PIECE)
      {
        [[likely]]
        if (empty)
        {
          fmt::format_to(s, "{}", util::toChar(empty));
          empty = 0;
        }
        fmt::format_to(s, "{}", piece_letter[pc]);
      } else
        empty++;
    }

    [[likely]]
    if (empty)
      fmt::format_to(s, "{}", util::toChar(empty));

    [[likely]]
    if (r > 0)
      fmt::format_to(s, "/");
  }

  fmt::format_to(s, " {} ", pos->side_to_move == WHITE ? 'w' : 'b');

  [[unlikely]]
  if (can_castle())
  {
    [[unlikely]]
    if (can_castle(WHITE_OO))
      fmt::format_to(s, "K");

    [[unlikely]]
    if (can_castle(WHITE_OOO))
      fmt::format_to(s, "Q");

    [[unlikely]]
    if (can_castle(BLACK_OO))
      fmt::format_to(s, "k");

    [[unlikely]]
    if (can_castle(BLACK_OOO))
      fmt::format_to(s, "q");
  } else
    fmt::format_to(s, "-");

  [[unlikely]]
  if (const Square en_pessant_sq = en_passant_square(); en_pessant_sq != NO_SQ)
    fmt::format_to(s, " {} ", square_to_string(en_pessant_sq));
  else
    fmt::format_to(s, " - ");

  fmt::format_to(s, "{} {}", pos->rule50, 1 + (plies - (pos->side_to_move == BLACK)) / 2);

  return fmt::to_string(buf);
}

void Board::setup_castling(const std::string_view s)
{
  castle_rights_mask.fill(NO_CASTLING);

  [[likely]]
  if (s.front() == '-')
    return;

  for (const char c : s)
  {
    const Color us   = static_cast<Color>(!std::isupper(c));
    const char token = std::tolower(c);

    if (token == 'k')
      add_castle_rights<KING_SIDE>(us, std::nullopt);
    else if (token == 'q')
      add_castle_rights<QUEEN_SIDE>(us, std::nullopt);
    else if (util::inBetween<'a', 'h'>(token))
    {
      chess960                            = true;
      const std::optional<File> rook_file = std::make_optional(static_cast<File>(token - 'a'));

      if (rook_file.value() > file_of(square<KING>(us)))
        add_castle_rights<KING_SIDE>(us, rook_file);
      else
        add_castle_rights<QUEEN_SIDE>(us, rook_file);
    }
  }
}

std::string Board::move_to_string(const Move m) const
{
  const MoveType mt = type_of(m);

  // shredder fen
  [[unlikely]]
  if (chess960 && mt & CASTLE)
    return fmt::format("{}{}", square_to_string(move_from(m)), square_to_string(rook_castles_from[move_to(m)]));

  [[unlikely]]
  if (mt & PROMOTION)
    return fmt::format(
      "{}{}{}",
      square_to_string(move_from(m)),
      square_to_string(move_to(m)),
      piece_to_string(type_of(move_promoted(m))));

  return fmt::format("{}{}", square_to_string(move_from(m)), square_to_string(move_to(m)));
}

void Board::print_moves()
{
  MoveList<LEGALMOVES> ml = MoveList<LEGALMOVES>(this);
  for (u8 i = 1; const auto m : ml)
    fmt::print("{}. {}   {}\n", i++, move_to_string(m.move), m.score);
}

void Board::update_position(Position *p) const
{
  p->checkers  = attackers_to(square<KING>(p->side_to_move)) & pieces(~p->side_to_move);
  p->in_check  = is_attacked(square<KING>(p->side_to_move), ~p->side_to_move);
  Key key      = zobrist.zero();
  Key pawn_key = zobrist.noPawn();
  Bitboard b   = pieces();

  while (b)
  {
    const Square sq = pop_lsb(&b);
    const Piece pc  = piece(sq);
    key ^= zobrist.pst(pc, sq);
    [[likely]]
    if (type_of(pc) == PAWN)
      pawn_key ^= zobrist.pst(pc, sq);

    p->material.add(pc);
  }

  [[unlikely]]
  if (p->en_passant_square != NO_SQ)
    key ^= zobrist.ep(file_of(p->en_passant_square));

  if (p->side_to_move != WHITE)
    key ^= zobrist.side();

  key ^= zobrist.castle(p->castle_rights);

  p->key                = key;
  p->pawn_structure_key = pawn_key;
}

Bitboard Board::attackers_to(const Square s, const Bitboard occ) const
{
  return (pawn_attacks_bb(BLACK, s) & pieces(PAWN, WHITE)) | (pawn_attacks_bb(WHITE, s) & pieces(PAWN, BLACK))
       | (piece_attacks_bb<KNIGHT>(s) & pieces(KNIGHT)) | (piece_attacks_bb<BISHOP>(s, occ) & pieces(BISHOP, QUEEN))
       | (piece_attacks_bb<ROOK>(s, occ) & pieces(ROOK, QUEEN)) | (piece_attacks_bb<KING>(s) & pieces(KING));
}

Bitboard Board::attackers_to(const Square s) const
{
  return attackers_to(s, pieces());
}

template<CastlingRight Side>
void Board::add_castle_rights(const Color us, std::optional<File> rook_file)
{
  const CastlingRight castle_rights = make_castling<Side>(us);
  const Rank rank_one               = relative_rank(us, RANK_1);
  const Square ksq                  = square<KING>(us);
  const Square rook_square =
    !rook_file.has_value() ? find_rook_square<Side>(us, this) : make_square(rook_file.value(), rank_one);
  constexpr File r_from_file = Side == KING_SIDE ? FILE_G : FILE_C;
  const Square rook_from     = make_square(r_from_file, rank_one);

  pos->castle_rights |= castle_rights;
  castle_rights_mask[rook_square] |= castle_rights;
  castle_rights_mask[ksq] |= castle_rights;
  rook_castles_from[rook_from] = rook_square;

  const Square kto = relative_square(us, castle_rights & KING_SIDE ? G1 : C1);
  const Square rto = relative_square(us, castle_rights & KING_SIDE ? F1 : D1);

  castling_path[castle_rights] = (between(rook_square, rto) | between(ksq, kto)) & ~bit(ksq, rook_square);

  if constexpr (Side == KING_SIDE)
    oo_king_from[us] = ksq;
  else
    ooo_king_from[us] = ksq;

  [[unlikely]]
  if (file_of(ksq) != FILE_E)
    chess960 = true;
  else
  {
    constexpr File far_file = Side == KING_SIDE ? FILE_H : FILE_A;

    if (file_of(rook_square) != far_file)
      chess960 = true;
  }
}

bool Board::is_castleling_impeeded(const CastlingRight cr) const
{
  assert(cr == WHITE_OO || cr == WHITE_OOO || cr == BLACK_OO || cr == BLACK_OOO);
  return pieces() & castling_path[cr];
}

bool Board::gives_check(const Move m)
{
  perform_move(m);
  const bool attacked = is_attacked(square<KING>(~pos->side_to_move), pos->side_to_move);
  unperform_move(m);
  return attacked;
}

bool Board::is_legal(const Move m, const Piece pc, const Square from, const MoveType mt)
{
  if (!(pos->pinned & from) && !pos->in_check && type_of(pc) != KING && !(mt & EPCAPTURE))
    return true;

  perform_move(m);
  const bool attacked = is_attacked(square<KING>(pos->side_to_move), ~pos->side_to_move);
  unperform_move(m);
  return !attacked;
}

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