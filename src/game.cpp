#include "game.h"
#include <cctype>
#include <optional>
#include <cstring>
#include "zobrist.h"
#include "position.h"
#include "board.h"
#include "util.h"

namespace {

[[nodiscard]]
bool get_ep_square(const char **p, Square &sq) {
  if (**p == '-')
  {
    sq = no_square;
    return true;
  }

  if (!util::in_between(**p, 'a', 'h'))
    return false;

  (*p)++;

  if (!util::in_between(**p, '3', '6'))
    return false;

  sq = static_cast<Square>(*(*p - 1) - 'a' + (**p - '1') * 8);
  return true;
}

[[nodiscard]]
Color get_side_to_move(const char **p) {
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

[[nodiscard]]
bool get_delimiter(const char **p) {
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

  pos->castle_rights                                |= CastleRights;
  castle_rights_mask[flip[Them][rook_file.value()]] -= oo_allowed_mask[Us];
  castle_rights_mask[flip[Them][file_of(ksq)]]      -= oo_allowed_mask[Us];
  rook_castles_from[flip[Them][g1]]                  = flip[Them][rook_file.value()];
  oo_king_from[Us]                                   = ksq;

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

  pos->castle_rights                                |= CastleRights;
  castle_rights_mask[flip[Them][rook_file.value()]] -= ooo_allowed_mask[Us];
  castle_rights_mask[flip[Them][file_of(ksq)]]      -= ooo_allowed_mask[Us];
  rook_castles_from[flip[Them][c1]]                  = flip[Them][rook_file.value()];
  ooo_king_from[Us]                                  = ksq;

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

Game::Game()
  : position_list(new Position[2000]), pos(position_list), chess960(false), xfen(false) {
  for (auto i = 0; i < 2000; i++)
    position_list[i].b = &board;
}

Game::~Game() { delete[] position_list; }

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
      for (auto bb = board.piece[piece | side << 3]; bb != 0; reset_lsb(bb))
        key ^= zobrist::zobrist_pst[piece | side << 3][lsb(bb)];
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

  while ((num_moves = num_moves - 2) >= 0 && prev - position_list > 1)
  {
    prev -= 2;

    if (prev->key == pos->key)
      return true;
  }
  return false;
}

int Game::half_move_count() const {
  // TODO : fix implementation defined behaviour
  return pos - position_list;
}

void Game::add_piece(const int p, const Color c, const Square sq) {
  const auto pc = p | c << 3;

  board.add_piece(p, c, sq);
  pos->key ^= zobrist::zobrist_pst[pc][sq];

  if (p == Pawn)
    pos->pawn_structure_key ^= zobrist::zobrist_pst[pc][sq];

  pos->material.add(pc);
}

int Game::new_game(const char *fen) {
  if (set_fen(fen) == 0)
    return 0;

  return set_fen(kStartPosition.data());
}

int Game::set_fen(const char *fen) {
  pos = position_list;
  pos->clear();
  board.clear();

  const auto *p = fen;
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

char *Game::get_fen() const {
  constexpr std::string_view piece_letter = "PNBRQK  pnbrqk";
  static char fen[128];
  auto *p = fen;
  char buf[12];

  std::memset(p, 0, 128);

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
          *p++  = empty + '0';
          empty = 0;
        }
        *p++ = piece_letter[pc];
      } else
        empty++;
    }

    if (empty)
      *p++ = empty + '0';

    if (r > 0)
      *p++ = '/';
  }
  std::memcpy(p, pos->side_to_move == WHITE ? " w " : " b ", 3);
  p += 3;

  if (pos->castle_rights == 0)
  {
    std::memcpy(p, "- ", 2);
    p += 2;
  } else
  {
    if (pos->castle_rights & 1)
      *p++ = 'K';

    if (pos->castle_rights & 2)
      *p++ = 'Q';

    if (pos->castle_rights & 4)
      *p++ = 'k';

    if (pos->castle_rights & 8)
      *p++ = 'q';

    *p++ = ' ';
  }

  if (pos->en_passant_square != no_square)
  {
    std::memcpy(p, square_to_string(pos->en_passant_square, buf), 2);
    p += 2;
    *p++ = ' ';
  } else
  {
    std::memcpy(p, "- ", 2);
    p += 2;
  }
  std::memset(buf, 0, 12);
  std::memcpy(p, _itoa(pos->reversible_half_move_count, buf, 10), 12);
  p[strlen(p)] = ' ';
  std::memset(buf, 0, 12);
  std::memcpy(p + strlen(p), _itoa(static_cast<int>((pos - position_list) / 2 + 1), buf, 10), 12);
  return fen;
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

    if (util::in_between(c, 'A', 'H'))
    {
      chess960 = true;
      xfen     = false;

      const auto rook_file = std::optional<File>(static_cast<File>(c - 'A'));

      if (rook_file.value() > file_of(board.king_square[WHITE]))
        add_short_castle_rights<WHITE>(pos, this, board, rook_file);
      else
        add_long_castle_rights<WHITE>(pos, this, board, rook_file);
    } else if (util::in_between<char>(c, 'a', 'h'))
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

  pos += other->pos - other->position_list;

  for (auto i = 0; i < 2000; i++)
  {
    position_list[i]   = other->position_list[i];
    position_list[i].b = &board;
  }
}

const char *Game::move_to_string(const uint32_t m, char *buf) const {
  char tmp1[12];
  char tmp2[12];
  char tmp3[12];

  if (is_castle_move(m) && chess960)
  {
    if (xfen && move_to(m) == ooo_king_to[move_side(m)])
      strcpy(buf, "O-O-O");
    else if (xfen)
      strcpy(buf, "O-O");
    else// shredder fen
      sprintf(buf, "%s%s", square_to_string(move_from(m), tmp1), square_to_string(rook_castles_from[move_to(m)], tmp2));
  } else
  {
    sprintf(buf, "%s%s", square_to_string(move_from(m), tmp1), square_to_string(move_to(m), tmp2));

    if (is_promotion(m))
      sprintf(&buf[strlen(buf)], "%s", piece_to_string(move_promoted(m) & 7, tmp3));
  }
  return buf;
}

void Game::print_moves() const {
  auto i = 0;
  char buf[12];

  while (const MoveData *m = pos->next_move())
  {
    printf("%d. ", i++ + 1);
    printf("%s", move_to_string(m->move, buf));
    printf("   %d\n", m->score);
  }
}
