#include <cctype>
#include <optional>
#include <cstring>
#include <string>
#include <fmt/format.h>
#include "game.h"
#include "zobrist.h"
#include "board.h"
#include "util.h"
#include "transpositional.h"
#include "miscellaneous.h"

namespace {

[[nodiscard]] bool get_ep_square(const char **p, Square &sq) {
  if (**p == '-')
  {
    sq = no_square;
    return true;
  }

  if (!util::in_between<'a', 'h'>(**p))
    return false;

  (*p)++;

  if (!util::in_between<'3', '6'>(**p))
    return false;

  sq = static_cast<Square>(*(*p - 1) - 'a' + (**p - '1') * 8);
  return true;
}

[[nodiscard]] Color get_side_to_move(const char **p) {
  switch (**p)
  {
  case 'w':
    return WHITE;

  case 'b':
    return BLACK;

  default:
    break;
  }
  return COL_NB;
}

[[nodiscard]] bool get_delimiter(const char **p) {
  if (**p != ' ')
    return false;

  (*p)++;
  return true;
}

template<Color Us>
void add_short_castle_rights(Position *pos, Game *g, const Board &board, std::optional<File> rook_file) {

  constexpr auto Them         = ~Us;
  constexpr auto CastleRights = Us == WHITE ? 1 : 4;
  const auto ksq              = board.king_square[Us];

  if (!rook_file.has_value())
  {
    for (const auto f : ReverseFiles)
    {
      if (board.get_piece_type(static_cast<Square>(f + Us * 56)) == Rook)
      {
        rook_file = f;// right outermost rook for side
        break;
      }
    }
    g->xfen = true;
  }

  pos->castle_rights |= CastleRights;
  castle_rights_mask[flip[Them][rook_file.value()]] -= oo_allowed_mask[Us];
  castle_rights_mask[flip[Them][file_of(ksq)]] -= oo_allowed_mask[Us];
  rook_castles_from[flip[Them][g1]] = flip[Them][rook_file.value()];
  oo_king_from[Us]                  = ksq;

  if (file_of(ksq) != FILE_E || rook_file.value() != FILE_H)
    g->chess960 = true;
}

template<Color Us>
void add_long_castle_rights(Position *pos, Game *g, const Board &board, std::optional<File> rook_file) {

  constexpr auto Them         = ~Us;
  constexpr auto CastleRights = Us == WHITE ? 2 : 8;
  const auto ksq              = board.king_square[Us];

  if (!rook_file.has_value())
  {
    for (const auto f : Files)
    {
      if (board.get_piece_type(static_cast<Square>(f + Us * 56)) == Rook)
      {
        rook_file = f;// left outermost rook for side
        break;
      }
    }
    g->xfen = true;
  }

  pos->castle_rights |= CastleRights;
  castle_rights_mask[flip[Them][rook_file.value()]] -= ooo_allowed_mask[Us];
  castle_rights_mask[flip[Them][file_of(ksq)]] -= ooo_allowed_mask[Us];
  rook_castles_from[flip[Them][c1]] = flip[Them][rook_file.value()];
  ooo_king_from[Us]                 = ksq;

  if (file_of(ksq) != FILE_E || rook_file.value() != FILE_A)
    g->chess960 = true;
}

void update_key(Position *pos, const uint32_t m) {
  pos->key ^= pos->pawn_structure_key;
  pos->pawn_structure_key ^= zobrist::zobrist_side;

  if ((pos - 1)->en_passant_square != no_square)
    pos->key ^= zobrist::zobrist_ep_file[file_of((pos - 1)->en_passant_square)];

  if (pos->en_passant_square != no_square)
    pos->key ^= zobrist::zobrist_ep_file[file_of(pos->en_passant_square)];

  if (!m)
  {
    pos->key ^= pos->pawn_structure_key;
    return;
  }

  // from and to for moving piece
  if ((move_piece(m) & 7) == Pawn)
    pos->pawn_structure_key ^= zobrist::zobrist_pst[move_piece(m)][move_from(m)];
  else
    pos->key ^= zobrist::zobrist_pst[move_piece(m)][move_from(m)];

  if (move_type(m) & PROMOTION)
    pos->key ^= zobrist::zobrist_pst[move_promoted(m)][move_to(m)];
  else
  {
    if ((move_piece(m) & 7) == Pawn)
      pos->pawn_structure_key ^= zobrist::zobrist_pst[move_piece(m)][move_to(m)];
    else
      pos->key ^= zobrist::zobrist_pst[move_piece(m)][move_to(m)];
  }

  // remove captured piece
  if (is_ep_capture(m))
  {
    pos->pawn_structure_key ^= zobrist::zobrist_pst[move_captured(m)][move_to(m) + pawn_push(pos->side_to_move)];
  } else if (is_capture(m))
  {
    if ((move_captured(m) & 7) == Pawn)
      pos->pawn_structure_key ^= zobrist::zobrist_pst[move_captured(m)][move_to(m)];
    else
      pos->key ^= zobrist::zobrist_pst[move_captured(m)][move_to(m)];
  }

  // castling rights
  if ((pos - 1)->castle_rights != pos->castle_rights)
  {
    pos->key ^= zobrist::zobrist_castling[(pos - 1)->castle_rights];
    pos->key ^= zobrist::zobrist_castling[pos->castle_rights];
  }

  // rook move in castle
  if (is_castle_move(m))
  {
    const auto piece = Rook + side_mask(m);
    pos->key ^= zobrist::zobrist_pst[piece][rook_castles_from[move_to(m)]];
    pos->key ^= zobrist::zobrist_pst[piece][rook_castles_to[move_to(m)]];
  }
  pos->key ^= pos->pawn_structure_key;
}

}// namespace

Game::Game() : pos(position_list.data()), chess960(false), xfen(false) {
  for (auto &p : position_list)
    p.b = &board;
}

bool Game::make_move(const uint32_t m, const bool check_legal, const bool calculate_in_check) {
  if (m == 0)
    return make_null_move();

  board.make_move(m);

  if (check_legal && !(move_type(m) & CASTLE))
  {
    if (board.is_attacked(board.king_square[pos->side_to_move], ~pos->side_to_move))
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
  pos->reversible_half_move_count = is_capture(m) || (move_piece(m) & 7) == Pawn ? 0 : prev->reversible_half_move_count + 1;
  pos->en_passant_square          = move_type(m) & DOUBLEPUSH ? move_to(m) + pawn_push(pos->side_to_move) : no_square;
  pos->key                        = prev->key;
  pos->pawn_structure_key         = prev->pawn_structure_key;
  if (calculate_in_check)
    pos->in_check = board.is_attacked(board.king_square[pos->side_to_move], ~pos->side_to_move);
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
  pos->last_move                  = 0;
  pos->in_check                   = false;
  pos->castle_rights              = prev->castle_rights;
  pos->null_moves_in_row          = prev->null_moves_in_row + 1;
  pos->reversible_half_move_count = 0;
  pos->en_passant_square          = no_square;
  pos->key                        = prev->key;
  pos->pawn_structure_key         = prev->pawn_structure_key;
  update_key(pos, 0);
  return true;
}

uint64_t Game::calculate_key() {
  uint64_t key = 0;

  for (const auto piece : PieceTypes)
  {
    for (const auto side : Colors)
    {
      const auto pc = piece | (side << 3);
      for (auto bb = board.piece[pc]; bb != 0; reset_lsb(bb))
        key ^= zobrist::zobrist_pst[pc][lsb(bb)];
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

int Game::half_move_count() const {
  // TODO : fix implementation defined behaviour
  return pos - position_list.data();
}

void Game::add_piece(const int p, const Color c, const Square sq) {
  const auto pc = p | c << 3;

  board.add_piece(p, c, sq);
  pos->key ^= zobrist::zobrist_pst[pc][sq];

  if (p == Pawn)
    pos->pawn_structure_key ^= zobrist::zobrist_pst[pc][sq];

  pos->material.add(pc);
}

int Game::new_game(const std::string_view fen) {
  if (set_fen(fen) == 0)
    return 0;

  return set_fen(kStartPosition.data());
}

int Game::set_fen(const std::string_view fen) {
  pos = position_list.data();
  pos->clear();
  board.clear();

  const auto *p = fen.data();
  char f        = 1;
  char r        = 8;

  while (*p != 0 && !(f == 9 && r == 1))
  {
    if (isdigit(*p))
    {
      f += *p - '0';

      if (f > 9)
        return 1;
      p++;
      continue;
    }

    if (*p == '/')
    {
      if (f != 9)
        return 2;

      r--;
      f = 1;
      p++;
      continue;
    }

    const auto color = islower(*p) ? BLACK : WHITE;

    int piece;

    switch (toupper(*p))
    {
    case 'R':
      piece = Rook;
      break;

    case 'N':
      piece = Knight;
      break;

    case 'B':
      piece = Bishop;
      break;

    case 'Q':
      piece = Queen;
      break;

    case 'K':
      piece = King;
      break;

    case 'P':
      piece = Pawn;
      break;

    default:
      return 3;
    }
    add_piece(piece, color, static_cast<Square>(f - 1 + (r - 1) * 8));
    f++;
    p++;
  }

  if (!get_delimiter(&p) || (pos->side_to_move = get_side_to_move(&p)) == COL_NB)
    return 4;
  p++;

  if (!get_delimiter(&p) || setup_castling(&p) == -1)
    return 5;

  if (!get_delimiter(&p))
    return 6;

  Square sq;
  if (!get_ep_square(&p, sq))
    return 6;

  pos->en_passant_square          = sq;// < 64 ? sq : no_square;
  pos->reversible_half_move_count = 0;

  if (pos->side_to_move == BLACK)
  {
    pos->key ^= zobrist::zobrist_side;
    pos->pawn_structure_key ^= zobrist::zobrist_side;
  }
  pos->key ^= zobrist::zobrist_castling[pos->castle_rights];

  if (pos->en_passant_square != no_square)
    pos->key ^= zobrist::zobrist_ep_file[file_of(pos->en_passant_square)];

  pos->in_check = board.is_attacked(board.king_square[pos->side_to_move], ~pos->side_to_move);
  return 0;
}

std::string Game::get_fen() const {
  constexpr std::string_view piece_letter = "PNBRQK  pnbrqk";
  std::string s;

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
          s += empty + '0';
          empty = 0;
        }
        s += piece_letter[pc];
      } else
        empty++;
    }

    if (empty)
      s += empty + '0';

    if (r > 0)
      s += '/';
  }
  s += pos->side_to_move == WHITE ? " w " : " b ";

  if (pos->castle_rights == 0)
  {
    s += "- ";
  } else
  {
    if (pos->castle_rights & 1)
      s += 'K';

    if (pos->castle_rights & 2)
      s += 'Q';

    if (pos->castle_rights & 4)
      s += 'k';

    if (pos->castle_rights & 8)
      s += 'q';

    s += ' ';
  }

  if (pos->en_passant_square != no_square)
  {
    s += square_to_string(pos->en_passant_square);
    s += ' ';
  } else
    s += "- ";

  s += std::to_string(pos->reversible_half_move_count);
  s += ' ';
  s += std::to_string(static_cast<int>((pos - position_list.data()) / 2 + 1));

  return s;
}

int Game::setup_castling(const char **p) {
  castle_rights_mask.fill(15);

  if (**p == '-')
  {
    (*p)++;
    return 0;
  }

  while (**p != 0 && **p != ' ')
  {
    const auto c = **p;

    if (util::in_between<'A', 'H'>(c))
    {
      chess960 = true;
      xfen     = false;

      const auto rook_file = std::optional<File>(static_cast<File>(c - 'A'));

      if (rook_file.value() > file_of(board.king_square[WHITE]))
        add_short_castle_rights<WHITE>(pos, this, board, rook_file);
      else
        add_long_castle_rights<WHITE>(pos, this, board, rook_file);
    } else if (util::in_between<'a', 'h'>(c))
    {
      chess960 = true;
      xfen     = false;

      const auto rook_file = std::optional<File>(static_cast<File>(c - 'a'));

      if (rook_file.value() > file_of(board.king_square[BLACK]))
        add_short_castle_rights<BLACK>(pos, this, board, rook_file);
      else
        add_long_castle_rights<BLACK>(pos, this, board, rook_file);
    } else
    {
      if (c == 'K')
        add_short_castle_rights<WHITE>(pos, this, board, std::nullopt);
      else if (c == 'Q')
        add_long_castle_rights<WHITE>(pos, this, board, std::nullopt);
      else if (c == 'k')
        add_short_castle_rights<BLACK>(pos, this, board, std::nullopt);
      else if (c == 'q')
        add_long_castle_rights<BLACK>(pos, this, board, std::nullopt);
      else if (c != '-')
        return -1;
    }
    (*p)++;
  }
  return 0;
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

std::string Game::move_to_string(const uint32_t m) const {

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
    s += piece_to_string(move_promoted(m) & 7);
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
