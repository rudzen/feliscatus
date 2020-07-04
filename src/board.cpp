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

#include <optional>
#include <string>

#include <fmt/format.h>

#include "board.h"
#include "magic.h"
#include "util.h"
#include "transpositional.h"
#include "miscellaneous.h"
#include "prng.h"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>


namespace zobrist {

constexpr Key seed = 1070372;
constexpr Key zero = 0;
Key zobrist_pst[PIECE_NB][SQ_NB]{};
std::array<Key, CASTLING_RIGHT_NB> zobrist_castling{};
std::array<Key, FILE_NB> zobrist_ep_file{};
Key zobrist_side, zobrist_nopawn{};

}

namespace {

constexpr auto max_log_file_size = 1048576 * 5;
constexpr auto max_log_files     = 3;

std::shared_ptr<spdlog::logger> logger = spdlog::rotating_logger_mt("castleling_logger", "logs/castleling.txt", max_log_file_size, max_log_files);

[[nodiscard]] std::optional<Square> get_ep_square(std::string_view s) {
  if (s.empty() || s.length() == 1 || s.front() == '-')
    return std::make_optional(NO_SQ);

  const auto first = s.front();

  if (!util::in_between<'a', 'h'>(first))
    return std::nullopt;

  s.remove_prefix(1);

  return !util::in_between<'3', '6'>(s.front()) ? std::nullopt : std::make_optional(static_cast<Square>(first - 'a' + (s.front() - '1') * 8));
}

template<Color Us>
void add_short_castle_rights(Board *b, std::optional<File> rook_file) {

  constexpr auto CastleRights = Us == WHITE ? WHITE_OO : BLACK_OO;
  constexpr auto Rank_1       = relative_rank(Us, RANK_1);
  const auto ksq              = b->king_sq(Us);

  if (!rook_file.has_value())
  {
    for (const auto f : ReverseFiles)
    {
      if (b->get_piece_type(make_square(f, Rank_1)) == ROOK)
      {
        rook_file = f;// right outermost rook for side
        break;
      }
    }
    b->xfen = true;
  }

  b->pos->castle_rights |= CastleRights;

  const auto rook_square = make_square(rook_file.value(), Rank_1);

  b->castle_rights_mask[rook_square] -= oo_allowed_mask[Us];
  b->castle_rights_mask[ksq] -= oo_allowed_mask[Us];
  rook_castles_from[make_square(FILE_G, Rank_1)] = rook_square;
  oo_king_from[Us]                               = ksq;

  if (file_of(ksq) != FILE_E || rook_file.value() != FILE_H)
    b->chess960 = true;
}

template<Color Us>
void add_long_castle_rights(Board *b, std::optional<File> rook_file) {

  // constexpr auto Them         = ~Us;
  constexpr auto CastleRights = Us == WHITE ? WHITE_OOO : BLACK_OOO;
  constexpr auto Rank_1       = relative_rank(Us, RANK_1);
  const auto ksq              = b->king_sq(Us);

  if (!rook_file.has_value())
  {
    for (const auto f : Files)
    {
      if (b->get_piece_type(make_square(f, Rank_1)) == ROOK)
      {
        rook_file = f;// left outermost rook for side
        break;
      }
    }
    b->xfen = true;
  }

  b->pos->castle_rights |= CastleRights;

  const auto rook_square = make_square(rook_file.value(), Rank_1);

  b->castle_rights_mask[rook_square] -= ooo_allowed_mask[Us];
  b->castle_rights_mask[ksq] -= ooo_allowed_mask[Us];
  rook_castles_from[make_square(FILE_C, Rank_1)] = rook_square;
  ooo_king_from[Us]                              = ksq;

  if (file_of(ksq) != FILE_E || rook_file.value() != FILE_A)
    b->chess960 = true;
}

void update_key(Position *pos, const Move m) {
  auto pawn_key = pos->pawn_structure_key;
  auto key      = pos->key ^ pawn_key;

  pawn_key ^= zobrist::zobrist_side;
  const auto *prev = (pos - 1);

  if (prev->en_passant_square != NO_SQ)
    key ^= zobrist::zobrist_ep_file[file_of(prev->en_passant_square)];

  if (pos->en_passant_square != NO_SQ)
    key ^= zobrist::zobrist_ep_file[file_of(pos->en_passant_square)];

  if (!m)
  {
    key ^= pawn_key;
    pos->key                = key;
    pos->pawn_structure_key = pawn_key;
    return;
  }

  const auto piece   = move_piece(m);
  const auto is_pawn = type_of(piece) == PAWN;
  const auto from    = move_from(m);
  const auto to      = move_to(m);
  const auto mt      = type_of(m);

  // from and to for moving piece
  if (is_pawn)
    pawn_key ^= zobrist::zobrist_pst[piece][from];
  else
    key ^= zobrist::zobrist_pst[piece][from];

  if (mt & PROMOTION)
    key ^= zobrist::zobrist_pst[move_promoted(m)][to];
  else
  {
    if (is_pawn)
      pawn_key ^= zobrist::zobrist_pst[piece][to];
    else
      key ^= zobrist::zobrist_pst[piece][to];
  }

  // remove captured piece
  if (mt & EPCAPTURE)
    pawn_key ^= zobrist::zobrist_pst[move_captured(m)][to + pawn_push(pos->side_to_move)];
  else if (mt & CAPTURE)
  {
    if (is_pawn)
      pawn_key ^= zobrist::zobrist_pst[move_captured(m)][to];
    else
      key ^= zobrist::zobrist_pst[move_captured(m)][to];
  }

  // castling rights
  if (prev->castle_rights != pos->castle_rights)
    key ^= zobrist::zobrist_castling[prev->castle_rights] ^ zobrist::zobrist_castling[pos->castle_rights];

  // rook move in castle
  if (mt & CASTLE)
  {
    const auto rook = make_piece(ROOK, move_side(m));
    key ^= zobrist::zobrist_pst[rook][rook_castles_from[to]] ^ zobrist::zobrist_pst[rook][rook_castles_to[to]];
  }
  key ^= pawn_key;
  pos->key                = key;
  pos->pawn_structure_key = pawn_key;
}

}// namespace

Board::Board()
  : pos(position_list.data()), chess960(false), xfen(false) {
  for (auto &p : position_list)
    p.b = this;
}

Board::Board(const std::string_view fen, Data *data)
  : Board() {
  set_fen(fen, data);
}

void Board::init() {

  PRNG<Key> rng(zobrist::seed);

  zobrist::zobrist_side   = rng();
  zobrist::zobrist_nopawn = rng();

  for (auto &z : zobrist::zobrist_pst)
    for (auto &z_pst : z)
      z_pst = rng();

  for (auto &z : zobrist::zobrist_castling)
    z = rng();

  for (auto &z : zobrist::zobrist_ep_file)
    z = rng();
}

void Board::clear() {
  occupied_by_side.fill(ZeroBB);
  occupied_by_type.fill(ZeroBB);
  board.fill(NO_PIECE);
  king_square.fill(NO_SQ);
  max_ply = plies = search_depth = 0;
}

void Board::perform_move(const Move m) {

  const auto from = move_from(m);
  const auto to   = move_to(m);
  const auto mt   = type_of(m);
  auto pc         = move_piece(m);

  [[unlikely]]
  if (mt & CASTLE)
  {
    const auto rook = make_piece(ROOK, move_side(m));
    remove_piece(rook_castles_from[to]);
    remove_piece(from);
    add_piece(rook, rook_castles_to[to]);
    add_piece(pc, to);
  } else
  {
    remove_piece(from);

    if (mt & EPCAPTURE)
    {
      const auto direction = pawn_push(color_of(pc));
      remove_piece(to - direction);
    } else if (mt & CAPTURE)
      remove_piece(to);

    if (mt & PROMOTION)
      pc = move_promoted(m);

    add_piece(pc, to);
  }

  if (type_of(pc) == KING)
    king_square[move_side(m)] = to;
}

void Board::unperform_move(const Move m) {

  const auto from = move_from(m);
  const auto to   = move_to(m);
  const auto pc   = move_piece(m);
  const auto mt   = type_of(m);

  if (mt & CASTLE)
  {
    const auto rook = make_piece(ROOK, move_side(m));
    remove_piece(to);
    remove_piece(rook_castles_to[to]);
    add_piece(pc, from);
    add_piece(rook, rook_castles_from[to]);
  } else
  {
    remove_piece(to);

    if (mt & EPCAPTURE)
    {
      const auto direction = pawn_push(color_of(pc));
      add_piece(move_captured(m), to - direction);
    } else if (mt & CAPTURE)
      add_piece(move_captured(m), to);

    add_piece(pc, from);
  }

  if (type_of(pc) == KING)
    king_square[move_side(m)] = from;
}

Bitboard Board::get_pinned_pieces(const Color c, const Square s) const {
  const auto them        = ~c;
  const auto all_pieces  = pieces();
  const auto side_pieces = pieces(c);
  auto pinners           = xray_bishop_attacks(all_pieces, side_pieces, s) & pieces(BISHOP, QUEEN, them);
  auto pinned_pieces     = ZeroBB;

  while (pinners)
    pinned_pieces |= between_bb[pop_lsb(&pinners)][s] & side_pieces;

  pinners = xray_rook_attacks(all_pieces, side_pieces, s) & pieces(ROOK, QUEEN, them);

  while (pinners)
    pinned_pieces |= between_bb[pop_lsb(&pinners)][s] & side_pieces;

  return pinned_pieces;
}

bool Board::is_attacked_by_slider(const Square s, const Color c) const {
  const auto all_pieces = pieces();
  const auto r_attacks  = piece_attacks_bb<ROOK>(s, all_pieces);

  if (pieces(ROOK, c) & r_attacks)
    return true;

  const auto b_attacks = piece_attacks_bb<BISHOP>(s, all_pieces);

  if (pieces(BISHOP, c) & b_attacks)
    return true;

  return pieces(QUEEN, c) & (b_attacks | r_attacks);
}

void Board::print() const {
  constexpr std::string_view piece_letter = "PNBRQK. pnbrqk. ";

  fmt::memory_buffer s;

  format_to(s, "\n");

  for (const Rank rank : ReverseRanks)
  {
    format_to(s, "{}  ", rank + 1);

    for (const auto file : Files)
    {
      const auto sq = make_square(file, rank);
      const auto pc = get_piece(sq);
      format_to(s, "{} ", piece_letter[pc]);
    }
    format_to(s, "\n");
  }

  fmt::print("{}   a b c d e f g h\n", fmt::to_string(s));
}

bool Board::is_passed_pawn_move(const Move m) const {
  return move_piece_type(m) == PAWN && is_pawn_passed(move_to(m), move_side(m));
}

bool Board::is_pawn_isolated(const Square s, const Color c) const {
  const auto f               = bb_file(file_of(s));
  const auto neighbour_files = shift_bb<WEST>(f) | shift_bb<EAST>(f);
  return (pieces(PAWN, c) & neighbour_files) == 0;
}

bool Board::is_pawn_behind(const Square s, const Color c) const {
  const auto bbsq = bit(s);
  return (pieces(PAWN, c) & pawn_fill[~c](shift_bb<WEST>(bbsq) | shift_bb<EAST>(bbsq))) == 0;
}

bool Board::make_move(const Move m, const bool check_legal, const bool calculate_in_check) {
  if (!m)
    return make_null_move();

  perform_move(m);

  const auto mt = type_of(m);

  if (check_legal && !(mt & CASTLE))
  {
    if (is_attacked(king_sq(pos->side_to_move), ~pos->side_to_move))
    {
      unperform_move(m);
      return false;
    }
  }
  auto *const prev                = pos++;
  pos->side_to_move               = ~prev->side_to_move;
  pos->material                   = prev->material;
  pos->last_move                  = m;
  pos->castle_rights              = prev->castle_rights & castle_rights_mask[move_from(m)] & castle_rights_mask[move_to(m)];
  pos->null_moves_in_row          = 0;
  pos->reversible_half_move_count = mt & (CAPTURE | EPCAPTURE) || type_of(move_piece(m)) == PAWN ? 0 : prev->reversible_half_move_count + 1;
  pos->en_passant_square          = type_of(m) & DOUBLEPUSH ? move_to(m) + pawn_push(pos->side_to_move) : NO_SQ;
  pos->key                        = prev->key;
  pos->pawn_structure_key         = prev->pawn_structure_key;
  if (calculate_in_check)
    pos->in_check = is_attacked(king_sq(pos->side_to_move), ~pos->side_to_move);
  if (pos->in_check)
    pos->checkers = attackers_to(king_sq(pos->side_to_move));

  update_key(pos, m);
  pos->material.make_move(m);

  prefetch(TT.find(pos->key));

  return true;
}

bool Board::make_move(const Move m, const bool check_legal) {
  return make_move(m, check_legal, gives_check(m));
}

void Board::unmake_move() {
  if (pos->last_move)
    unperform_move(pos->last_move);
  pos--;
}

bool Board::make_null_move() {
  auto *const prev                = pos++;
  pos->side_to_move               = ~prev->side_to_move;
  pos->material                   = prev->material;
  pos->last_move                  = MOVE_NONE;
  pos->in_check                   = false;
  pos->castle_rights              = prev->castle_rights;
  pos->null_moves_in_row          = prev->null_moves_in_row + 1;
  pos->reversible_half_move_count = 0;
  pos->en_passant_square          = NO_SQ;
  pos->key                        = prev->key;
  pos->pawn_structure_key         = prev->pawn_structure_key;
  update_key(pos, MOVE_NONE);
  return true;
}

uint64_t Board::calculate_key() const {
  auto key = zobrist::zero;

  for (const auto pt : PieceTypes)
  {
    for (const auto c : Colors)
    {
      auto bb = pieces(pt, c);
      while (bb)
      {
        const auto sq = pop_lsb(&bb);
        const auto pc = get_piece(sq);
        key ^= zobrist::zobrist_pst[pc][sq];
      }
    }
  }
  key ^= zobrist::zobrist_castling[pos->castle_rights];

  if (pos->en_passant_square != NO_SQ)
    key ^= zobrist::zobrist_ep_file[file_of(pos->en_passant_square)];

  if (pos->side_to_move == BLACK)
    key ^= zobrist::zobrist_side;

  return key;
}

bool Board::is_repetition() const {
  auto num_moves = pos->reversible_half_move_count;
  auto *prev     = pos;

  while ((num_moves = num_moves - 2) >= 0 && prev - position_list.data() > 1)
  {
    prev -= 2;

    if (prev->key == pos->key)
      return true;
  }
  return false;
}

int64_t Board::half_move_count() const {
  // TODO : fix implementation defined behaviour
  return pos - position_list.data();
}

int Board::new_game(Data *data) {
  return set_fen(start_position, data);
}

int Board::set_fen(std::string_view fen, Data *data) {
  pos = position_list.data();
  pos->clear();
  clear();

  constexpr std::string_view piece_index{"PNBRQK"};
  constexpr auto splitter = ' ';

  Square sq = A8;

  // indicates where in the fen the last space was located
  std::size_t space{};

  // updates the fen and the current view to the next part of the fen
  const auto update_current = [&fen, &space]()
  {
    // remove already parsed section from fen
    fen.remove_prefix(space);

    // adjust space to next
    space = fen.find_first_of(splitter);

    // slice current view
    return fen.substr(0, space);
  };

  // the current view of the fen
  auto current = update_current();

  for (const auto token : current)
  {
    [[unlikely]]
    if (std::isdigit(token))
      sq += util::from_char<int>(token) * EAST;
    else if (token == '/')
      sq += SOUTH * 2;
    else if (const auto pc_idx = piece_index.find_first_of(toupper(token)); pc_idx != std::string_view::npos)
    {
      add_piece(make_piece(static_cast<PieceType>(pc_idx), (islower(token) ? BLACK : WHITE)), sq);
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
  if (!setup_castling(current))
    return 5;

  // En-passant
  space++;
  current = update_current();
  if (const auto ep_sq = get_ep_square(current); ep_sq)
    pos->en_passant_square = ep_sq.value();
  else
    return 6;

  // TODO : Parse the rest

  pos->reversible_half_move_count = 0;

  update_position(pos);

  data_ = data;

  return 0;
}

std::string Board::get_fen() const {
  constexpr std::string_view piece_letter = "PNBRQK  pnbrqk";
  fmt::memory_buffer s;

  for (const Rank r : ReverseRanks)
  {
    auto empty = 0;

    for (const auto f : Files)
    {
      const auto sq = make_square(f, r);
      const auto pc = get_piece(sq);

      if (pc != NO_PIECE)
      {
        if (empty)
        {
          format_to(s, "{}", util::to_char(empty));
          empty = 0;
        }
        format_to(s, "{}", piece_letter[pc]);
      } else
        empty++;
    }

    if (empty)
      format_to(s, "{}", util::to_char(empty));

    if (r > 0)
      format_to(s, "/");
  }

  format_to(s, " {} ", pos->side_to_move ? 'w' : 'b');

  if (can_castle())
  {
    if (can_castle(WHITE_OO))
      format_to(s, "K");

    if (can_castle(WHITE_OOO))
      format_to(s, "Q");

    if (can_castle(BLACK_OO))
      format_to(s, "k");

    if (can_castle(BLACK_OOO))
      format_to(s, "q");
  } else
    format_to(s, "-");

  if (const auto en_pessant_sq = en_passant_square(); en_pessant_sq != NO_SQ)
    format_to(s, " {} ", square_to_string(en_pessant_sq));
  else
    format_to(s, " - ");

  format_to(s, "{} {}", pos->reversible_half_move_count, static_cast<int>((pos - position_list.data()) / 2 + 1));

  return fmt::to_string(s);
}

bool Board::setup_castling(const std::string_view s) {

  castle_rights_mask.fill(15);

  if (s.front() == '-')
    return true;

  for (const auto c : s)
  {
    if (c == 'K')
      add_short_castle_rights<WHITE>(this, std::nullopt);
    else if (c == 'Q')
      add_long_castle_rights<WHITE>(this, std::nullopt);
    else if (c == 'k')
      add_short_castle_rights<BLACK>(this, std::nullopt);
    else if (c == 'q')
      add_long_castle_rights<BLACK>(this, std::nullopt);
    else if (util::in_between<'A', 'H'>(c))
    {
      chess960 = true;
      xfen     = false;

      const auto rook_file = std::make_optional(static_cast<File>(c - 'A'));

      if (rook_file.value() > file_of(king_sq(WHITE)))
        add_short_castle_rights<WHITE>(this, rook_file);
      else
        add_long_castle_rights<WHITE>(this, rook_file);
    } else if (util::in_between<'a', 'h'>(c))
    {
      chess960 = true;
      xfen     = false;

      const auto rook_file = std::make_optional(static_cast<File>(c - 'a'));

      if (rook_file.value() > file_of(king_sq(BLACK)))
        add_short_castle_rights<BLACK>(this, rook_file);
      else
        add_long_castle_rights<BLACK>(this, rook_file);
    } else
      return false;
  }

  return true;
}

std::string Board::move_to_string(const Move m) const {

  const auto mt = type_of(m);

  [[unlikely]]
  if (mt & CASTLE && chess960)
  {
    if (xfen && move_to(m) == ooo_king_to[move_side(m)])
      return std::string("O-O-O");
    if (xfen)
      return std::string("O-O");
    // shredder fen
    return fmt::format("{}{}", square_to_string(move_from(m)), square_to_string(rook_castles_from[move_to(m)]));
  }
  auto s = fmt::format("{}{}", square_to_string(move_from(m)), square_to_string(move_to(m)));

  if (mt & PROMOTION)
    s += piece_to_string(type_of(move_promoted(m)));
  return s;
}

void Board::print_moves() const {
  auto i = 0;

  while (const MoveData *m = pos->next_move())
  {
    fmt::print("%{}. ", i++ + 1);
    fmt::print(move_to_string(m->move));
    fmt::print("   {}\n", m->score);
  }
}

void Board::update_position(Position *p) const {

  p->checkers   = attackers_to(king_sq(pos->side_to_move)) & pieces(~pos->side_to_move);
  p->in_check   = is_attacked(king_sq(pos->side_to_move), ~pos->side_to_move);
  auto key      = zobrist::zero;
  auto pawn_key = zobrist::zobrist_nopawn;
  auto b        = pieces();

  while (b)
  {
    const auto sq = pop_lsb(&b);
    const auto pc = get_piece(sq);
    key ^= zobrist::zobrist_pst[pc][sq];
    if (type_of(pc) == PAWN)
      pawn_key ^= zobrist::zobrist_pst[pc][sq];

    p->material.add(pc);
  }

  if (p->en_passant_square != NO_SQ)
    key ^= zobrist::zobrist_ep_file[file_of(p->en_passant_square)];

  if (p->side_to_move != WHITE)
    key ^= zobrist::zobrist_side;

  key ^= zobrist::zobrist_castling[p->castle_rights];

  p->key                = key;
  p->pawn_structure_key = pawn_key;
}

Bitboard Board::attackers_to(const Square s, const Bitboard occ) const {
  return (pawn_attacks_bb(BLACK, s) & pieces(PAWN, WHITE))
       | (pawn_attacks_bb(WHITE, s) & pieces(PAWN, BLACK))
       | (piece_attacks_bb<KNIGHT>(s) & pieces(KNIGHT))
       | (piece_attacks_bb<BISHOP>(s, occ) & pieces(BISHOP, QUEEN))
       | (piece_attacks_bb<ROOK>(s, occ) & pieces(ROOK, QUEEN))
       | (piece_attacks_bb<KING>(s) & pieces(KING));
}

Bitboard Board::attackers_to(const Square s) const {
  return attackers_to(s, pieces());
}

bool Board::gives_check(const Move m) {
  perform_move(m);
  const auto attacked = is_attacked(king_sq(~pos->side_to_move), pos->side_to_move);
  unperform_move(m);
  return attacked;
}

bool Board::is_legal(const Move m, const Piece pc, const Square from, const MoveType mt) {
  if (!(pos->pinned & from) && !pos->in_check && type_of(pc) != KING && !(mt & EPCAPTURE))
    return true;

  perform_move(m);
  const auto attacked = is_attacked(king_sq(pos->side_to_move), ~pos->side_to_move);
  unperform_move(m);
  return !attacked;
}
