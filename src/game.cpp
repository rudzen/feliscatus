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
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include "game.h"
#include "zobrist.h"
#include "util.h"
#include "transpositional.h"
#include "miscellaneous.h"

namespace {

constexpr auto max_log_file_size = 1048576 * 5;
constexpr auto max_log_files     = 3;

std::shared_ptr<spdlog::logger> logger = spdlog::rotating_logger_mt("castleling_logger", "logs/castleling.txt", max_log_file_size, max_log_files);

[[nodiscard]]
std::optional<Square> get_ep_square(std::string_view s) {
  if (s.empty() || s.length() == 1 || s.front() == '-')
    return std::make_optional(no_square);

  const auto first = s.front();

  if (!util::in_between<'a', 'h'>(first))
    return std::nullopt;

  s.remove_prefix(1);

  return !util::in_between<'3', '6'>(s.front())
       ? std::nullopt
       : std::make_optional(static_cast<Square>(first - 'a' + (s.front() - '1') * 8));
}

template<Color Us>
void add_short_castle_rights(Position *pos, Game *g, std::optional<File> rook_file) {

  constexpr auto CastleRights = Us == WHITE ? WHITE_OO : BLACK_OO;
  constexpr auto Rank_1       = relative_rank(Us, RANK_1);
  const auto ksq              = g->board.king_sq(Us);

  if (!rook_file.has_value())
  {
    for (const auto f : ReverseFiles)
    {
      if (g->board.get_piece_type(make_square(f, Rank_1)) == Rook)
      {
        rook_file = f;// right outermost rook for side
        break;
      }
    }
    g->xfen = true;
  }

  pos->castle_rights |= CastleRights;

  const auto rook_square = make_square(rook_file.value(), Rank_1);

  g->castle_rights_mask[rook_square] -= oo_allowed_mask[Us];
  g->castle_rights_mask[ksq] -= oo_allowed_mask[Us];
  rook_castles_from[make_square(FILE_G, Rank_1)] = rook_square;
  oo_king_from[Us]                  = ksq;

  if (file_of(ksq) != FILE_E || rook_file.value() != FILE_H)
    g->chess960 = true;
}

template<Color Us>
void add_long_castle_rights(Position *pos, Game *g, std::optional<File> rook_file) {

  //constexpr auto Them         = ~Us;
  constexpr auto CastleRights = Us == WHITE ? WHITE_OOO : BLACK_OOO;
  constexpr auto Rank_1       = relative_rank(Us, RANK_1);
  const auto ksq              = g->board.king_sq(Us);

  if (!rook_file.has_value())
  {
    for (const auto f : Files)
    {
      if (g->board.get_piece_type(make_square(f, Rank_1)) == Rook)
      {
        rook_file = f;// left outermost rook for side
        break;
      }
    }
    g->xfen = true;
  }

  pos->castle_rights |= CastleRights;

  const auto rook_square = make_square(rook_file.value(), Rank_1);

  g->castle_rights_mask[rook_square] -= ooo_allowed_mask[Us];
  g->castle_rights_mask[ksq] -= ooo_allowed_mask[Us];
  rook_castles_from[make_square(FILE_C, Rank_1)] = rook_square;
  ooo_king_from[Us]                 = ksq;

  if (file_of(ksq) != FILE_E || rook_file.value() != FILE_A)
    g->chess960 = true;
}

void update_key(Position *pos, const Move m) {
  pos->key ^= pos->pawn_structure_key;
  pos->pawn_structure_key ^= zobrist::zobrist_side;
  const auto *prev = (pos - 1);

  if (prev->en_passant_square != no_square)
    pos->key ^= zobrist::zobrist_ep_file[file_of(prev->en_passant_square)];

  if (pos->en_passant_square != no_square)
    pos->key ^= zobrist::zobrist_ep_file[file_of(pos->en_passant_square)];

  if (!m)
  {
    pos->key ^= pos->pawn_structure_key;
    return;
  }

  const auto piece = move_piece(m);
  const auto is_pawn = type_of(piece) == Pawn;
  const auto from = move_from(m);
  const auto to = move_to(m);

  // from and to for moving piece
  if (is_pawn)
    pos->pawn_structure_key ^= zobrist::zobrist_pst[piece][from];
  else
    pos->key ^= zobrist::zobrist_pst[piece][from];

  if (move_type(m) & PROMOTION)
    pos->key ^= zobrist::zobrist_pst[move_promoted(m)][to];
  else
  {
    if (is_pawn)
      pos->pawn_structure_key ^= zobrist::zobrist_pst[piece][to];
    else
      pos->key ^= zobrist::zobrist_pst[piece][to];
  }

  // remove captured piece
  if (is_ep_capture(m))
  {
    pos->pawn_structure_key ^= zobrist::zobrist_pst[move_captured(m)][to + pawn_push(pos->side_to_move)];
  } else if (is_capture(m))
  {
    if (is_pawn)
      pos->pawn_structure_key ^= zobrist::zobrist_pst[move_captured(m)][to];
    else
      pos->key ^= zobrist::zobrist_pst[move_captured(m)][to];
  }

  // castling rights
  if (prev->castle_rights != pos->castle_rights)
  {
    pos->key ^= zobrist::zobrist_castling[prev->castle_rights]
              ^ zobrist::zobrist_castling[pos->castle_rights];
  }

  // rook move in castle
  if (is_castle_move(m))
  {
    const auto rook = make_piece(Rook, move_side(m));
    pos->key ^= zobrist::zobrist_pst[rook][rook_castles_from[to]]
              ^ zobrist::zobrist_pst[rook][rook_castles_to[to]];
  }
  pos->key ^= pos->pawn_structure_key;
}

}// namespace

Game::Game() : pos(position_list.data()), chess960(false), xfen(false) {
  for (auto &p : position_list)
    p.b = &board;
}

Game::Game(std::string_view fen) : Game() {
  set_fen(fen);
}

bool Game::make_move(const Move m, const bool check_legal, const bool calculate_in_check) {
  if (m == 0)
    return make_null_move();

  board.make_move(m);

  if (check_legal && !(move_type(m) & CASTLE))
  {
    if (board.is_attacked(board.king_sq(pos->side_to_move), ~pos->side_to_move))
    {
      board.unmake_move(m);
      return false;
    }
  }
  auto *const prev                = pos++;
  pos->side_to_move               = ~prev->side_to_move;
  pos->material                   = prev->material;
  pos->last_move                  = m;
  pos->castle_rights              = prev->castle_rights & castle_rights_mask[move_from(m)] & castle_rights_mask[move_to(m)];
  pos->null_moves_in_row          = 0;
  pos->reversible_half_move_count = is_capture(m) || type_of(move_piece(m)) == Pawn ? 0 : prev->reversible_half_move_count + 1;
  pos->en_passant_square          = move_type(m) & DOUBLEPUSH ? move_to(m) + pawn_push(pos->side_to_move) : no_square;
  pos->key                        = prev->key;
  pos->pawn_structure_key         = prev->pawn_structure_key;
  if (calculate_in_check)
    pos->in_check = board.is_attacked(board.king_sq(pos->side_to_move), ~pos->side_to_move);
  update_key(pos, m);
  pos->material.make_move(m);

  prefetch(TT.find(pos->key));

  return true;
}

void Game::unmake_move() {
  if (pos->last_move)
    board.unmake_move(pos->last_move);
  pos--;
}

bool Game::make_null_move() {
  auto *const prev                = pos++;
  pos->side_to_move               = ~prev->side_to_move;
  pos->material                   = prev->material;
  pos->last_move                  = MOVE_NONE;
  pos->in_check                   = false;
  pos->castle_rights              = prev->castle_rights;
  pos->null_moves_in_row          = prev->null_moves_in_row + 1;
  pos->reversible_half_move_count = 0;
  pos->en_passant_square          = no_square;
  pos->key                        = prev->key;
  pos->pawn_structure_key         = prev->pawn_structure_key;
  update_key(pos, MOVE_NONE);
  return true;
}

uint64_t Game::calculate_key() {
  uint64_t key = 0;

  for (const auto pt : PieceTypes)
  {
    for (const auto c : Colors)
    {
      auto bb = board.pieces(pt, c);
      while (bb)
      {
        const auto sq = pop_lsb(&bb);
        const auto pc = board.get_piece(sq);
        key ^= zobrist::zobrist_pst[pc][sq];
      }
    }
  }
  key ^= zobrist::zobrist_castling[pos->castle_rights];

  if (pos->en_passant_square != no_square)
    key ^= zobrist::zobrist_ep_file[file_of(pos->en_passant_square)];

  if (pos->side_to_move == BLACK)
    key ^= zobrist::zobrist_side;

  return key;
}

bool Game::is_repetition() const {
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

int64_t Game::half_move_count() const {
  // TODO : fix implementation defined behaviour
  return pos - position_list.data();
}

int Game::new_game(const std::string_view fen) {
  if (set_fen(fen) == 0)
    return 0;

  return set_fen(kStartPosition.data());
}

int Game::set_fen(std::string_view fen) {
  pos = position_list.begin();
  pos->clear();
  board.clear();

  constexpr std::string_view piece_index{"PNBRQK"};
  constexpr char Splitter = ' ';

  Square sq = a8;

  // indicates where in the fen the last space was located
  std::size_t space{};

  // updates the fen and the current view to the next part of the fen
  const auto update_current = [&fen, &space]() {
    // remove already parsed section from fen
    fen.remove_prefix(space);

    // adjust space to next
    space = fen.find_first_of(Splitter);

    // slice current view
    return fen.substr(0, space);
  };

  // the current view of the fen
  auto current = update_current();

  for (const auto token : current)
  {
    if (std::isdigit(token))
      sq += util::from_char<int>(token) * EAST;
    else if (token == '/')
      sq += SOUTH * 2;
    else if (const auto pc_idx = piece_index.find_first_of(toupper(token)); pc_idx != std::string_view::npos)
    {
      board.add_piece(make_piece(static_cast<PieceType>(pc_idx), (islower(token) ? BLACK : WHITE)), sq);
      ++sq;
    }
  }

  // Side to move
  space++;
  current = update_current();
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

  pos->in_check = board.is_attacked(board.king_sq(pos->side_to_move), ~pos->side_to_move);
  return 0;
}

std::string Game::get_fen() const {
  constexpr std::string_view piece_letter = "PNBRQK  pnbrqk";
  fmt::memory_buffer s;

  for (const Rank r : ReverseRanks)
  {
    auto empty = 0;

    for (const auto f : Files)
    {
      const auto sq = make_square(f, r);
      const auto pc = board.get_piece(sq);

      if (pc != NoPiece)
      {
        if (empty)
        {
          fmt::format_to(s, "{}", util::to_char(empty));
          empty = 0;
        }
        fmt::format_to(s, "{}", piece_letter[pc]);
      } else
        empty++;
    }

    if (empty)
      fmt::format_to(s, "{}", util::to_char(empty));

    if (r > 0)
      fmt::format_to(s, "/");
  }

  fmt::format_to(s, " {} ", pos->side_to_move ? 'w' : 'b');

  if (pos->can_castle())
  {
    if (pos->can_castle(WHITE_OO))
      fmt::format_to(s, "K");

    if (pos->can_castle(WHITE_OOO))
      fmt::format_to(s, "Q");

    if (pos->can_castle(BLACK_OO))
      fmt::format_to(s, "k");

    if (pos->can_castle(BLACK_OOO))
      fmt::format_to(s, "q");
  }
  else
    fmt::format_to(s, "-");

  if (pos->en_passant_square != no_square)
    fmt::format_to(s, " {} ", square_to_string(pos->en_passant_square));
  else
    fmt::format_to(s, " - ");

  fmt::format_to(s, "{} {}", pos->reversible_half_move_count, static_cast<int>((pos - position_list.data()) / 2 + 1));

  return fmt::to_string(s);
}

bool Game::setup_castling(const std::string_view s) {

  castle_rights_mask.fill(15);

  if (s.front() == '-')
    return true;

  //position startpos moves e2e4 e7e5 g1f3 g8f6 f3e5 d7d6 e5f3 f6e4 d2d4 d6d5 f1d3 f8e7 c2c4 e7b4

  for (const auto c : s)
  {
    if (c == 'K')
      add_short_castle_rights<WHITE>(pos, this, std::nullopt);
    else if (c == 'Q')
      add_long_castle_rights<WHITE>(pos, this, std::nullopt);
    else if (c == 'k')
      add_short_castle_rights<BLACK>(pos, this, std::nullopt);
    else if (c == 'q')
      add_long_castle_rights<BLACK>(pos, this, std::nullopt);
    else if (util::in_between<'A', 'H'>(c))
    {
      chess960 = true;
      xfen     = false;

      const auto rook_file = std::make_optional(static_cast<File>(c - 'A'));

      if (rook_file.value() > file_of(board.king_sq(WHITE)))
        add_short_castle_rights<WHITE>(pos, this, rook_file);
      else
        add_long_castle_rights<WHITE>(pos, this, rook_file);
    } else if (util::in_between<'a', 'h'>(c))
    {
      chess960 = true;
      xfen     = false;

      const auto rook_file = std::make_optional(static_cast<File>(c - 'a'));

      if (rook_file.value() > file_of(board.king_sq(BLACK)))
        add_short_castle_rights<BLACK>(pos, this, rook_file);
      else
        add_long_castle_rights<BLACK>(pos, this, rook_file);
    } else
      return false;
  }

  return true;
}

void Game::copy(Game *other) {
  board    = other->board;
  chess960 = other->chess960;
  xfen     = other->xfen;

  pos += other->pos - other->position_list.data();

  for (std::size_t i = 0; auto &pl : position_list)
  {
    pl   = other->position_list[i++];
    pl.b = &board;
  }
}

std::string Game::move_to_string(const Move m) const {

  if (is_castle_move(m) && chess960)
  {
    if (xfen && move_to(m) == ooo_king_to[move_side(m)])
      return "O-O-O";
    if (xfen)
      return "O-O";
    // shredder fen
    return fmt::format("{}{}", square_to_string(move_from(m)), square_to_string(rook_castles_from[move_to(m)]));
  }
  auto s = fmt::format("{}{}", square_to_string(move_from(m)), square_to_string(move_to(m)));

  if (is_promotion(m))
    s += piece_to_string(type_of(move_promoted(m)));
  return s;
}

void Game::print_moves() const {
  auto i = 0;

  while (const MoveData *m = pos->next_move())
  {
    fmt::print("%{}. ", i++ + 1);
    fmt::print(move_to_string(m->move));
    fmt::print("   {}\n", m->score);
  }
}

void Game::update_position(Position *p) const {

  p->key = 0;
  p->pawn_structure_key = zobrist::zobrist_nopawn;

  auto b = board.pieces();

  while (b)
  {
    const auto sq = pop_lsb(&b);
    const auto pc = board.get_piece(sq);
    p->key ^= zobrist::zobrist_pst[pc][sq];
    if (type_of(pc) == Pawn)
      p->pawn_structure_key ^= zobrist::zobrist_pst[pc][sq];

    p->material.add(pc);
  }

  if (p->en_passant_square != no_square)
    p->key ^= zobrist::zobrist_ep_file[file_of(p->en_passant_square)];

  if (p->side_to_move != WHITE)
    p->key ^= zobrist::zobrist_side;

  p->key ^= zobrist::zobrist_castling[p->castle_rights];
}
