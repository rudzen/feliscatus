#pragma once

#include <cstdint>
#include <string_view>
#include "position.h"
#include "zobrist.h"
#include "types.h"

class Game {
public:
  Game() : position_list(new Position[2000]), pos(position_list), chess960(false), xfen(false) {
    for (auto i = 0; i < 2000; i++)
      position_list[i].board = &board;
  }

  virtual ~Game() { delete[] position_list; }

  bool make_move(const uint32_t m, const bool check_legal, const bool calculate_in_check) {
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
    update_key(m);
    pos->material.make_move(m);
    return true;
  }

  void unmake_move() {
    if (pos->last_move)
      board.unmake_move(pos->last_move);
    pos--;
  }

  bool make_null_move() {
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
    update_key(0);
    return true;
  }

  uint64_t calculate_key() {
    uint64_t key = 0;

    for (auto piece = Pawn; piece <= King; piece++)
    {
      for (auto side = 0; side <= 1; side++)
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

  void update_key(const uint32_t m) const {
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

  [[nodiscard]] bool is_repetition() const {
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

  // TODO : fix implementation defined behaviour
  [[nodiscard]] int half_move_count() const { return pos - position_list; }

  void add_piece(const int p, const Color c, const Square sq) {
    const auto pc = p | c << 3;

    board.add_piece(p, c, sq);
    pos->key ^= zobrist::zobrist_pst[pc][sq];

    if (p == Pawn)
      pos->pawn_structure_key ^= zobrist::zobrist_pst[pc][sq];

    pos->material.add(pc);
  }

  int new_game(const char *fen) {
    if (set_fen(fen) == 0)
      return 0;

    return set_fen(kStartPosition.data());
  }

  int set_fen(const char *fen) {
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

      auto color = islower(*p) ? BLACK : WHITE;

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

    if (pos->side_to_move == 1)
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

  [[nodiscard]] char *get_fen() const {
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
    memcpy(p, pos->side_to_move == WHITE ? " w " : " b ", 3);
    p += 3;

    if (pos->castle_rights == 0)
    {
      memcpy(p, "- ", 2);
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
      memcpy(p, square_to_string(pos->en_passant_square, buf), 2);
      p += 2;
      *p++ = ' ';
    } else
    {
      memcpy(p, "- ", 2);
      p += 2;
    }
    std::memset(buf, 0, 12);
    memcpy(p, _itoa(pos->reversible_half_move_count, buf, 10), 12);
    p[strlen(p)] = ' ';
    std::memset(buf, 0, 12);
    memcpy(p + strlen(p), _itoa(static_cast<int>((pos - position_list) / 2 + 1), buf, 10), 12);
    return fen;
  }

  [[nodiscard]] static bool get_ep_square(const char **p, Square &sq) {
    if (**p == '-')
    {
      sq = no_square;
      return true;
    }

    if (!in_between(**p, 'a', 'h'))
      return false;

    (*p)++;

    if (!in_between(**p, '3', '6'))
      return false;

    sq = static_cast<Square>(*(*p - 1) - 'a' + (**p - '1') * 8);
    return true;
  }

  [[nodiscard]] int setup_castling(const char **p) {
    for (const auto sq : Squares)
      castle_rights_mask[sq] = 15;

    if (**p == '-')
    {
      (*p)++;
      return 0;
    }

    while (**p != 0 && **p != ' ')
    {
      const auto c = **p;

      if (in_between<char>(c, 'A', 'H'))
      {
        chess960 = true;
        xfen     = false;

        const auto rook_file = c - 'A';

        if (rook_file > file_of(board.king_square[0]))
          add_short_castle_rights<WHITE>(rook_file);
        else
          add_long_castle_rights<WHITE>(rook_file);
      } else if (in_between<char>(c, 'a', 'h'))
      {
        chess960 = true;
        xfen     = false;

        const auto rook_file = c - 'a';

        if (rook_file > file_of(board.king_square[1]))
          add_short_castle_rights<BLACK>(rook_file);
        else
          add_long_castle_rights<BLACK>(rook_file);

      } else
      {
        if (c == 'K')
          add_short_castle_rights<WHITE>(-1);
        else if (c == 'Q')
          add_long_castle_rights<WHITE>(-1);
        else if (c == 'k')
          add_short_castle_rights<BLACK>(-1);
        else if (c == 'q')
          add_long_castle_rights<BLACK>(-1);
        else if (c != '-')
          return -1;
      }
      (*p)++;
    }
    return 0;
  }

  template<Color Us>
  void add_short_castle_rights(int rook_file) {

    constexpr auto Them         = ~Us;
    constexpr auto CastleRights = Us == WHITE ? 1 : 4;

    if (rook_file == -1)
    {
      for (auto i = 7; i >= 0; i--)
      {
        if (board.get_piece_type(static_cast<Square>(i + Us * 56)) == Rook)
        {
          rook_file = i;// right outermost rook for side
          break;
        }
      }
      xfen = true;
    }

    pos->castle_rights |= CastleRights;
    castle_rights_mask[flip[Them][rook_file]] -= oo_allowed_mask[Us];
    castle_rights_mask[flip[Them][file_of(board.king_square[Us])]] -= oo_allowed_mask[Us];
    rook_castles_from[flip[Them][g1]] = flip[Them][rook_file];
    oo_king_from[Us]                  = board.king_square[Us];

    if (file_of(board.king_square[Us]) != 4 || rook_file != 7)
      chess960 = true;
  }

  template<Color Us>
  void add_long_castle_rights(int rook_file) {

    constexpr auto Them        = ~Us;
    constexpr int CastleRights = Us == WHITE ? 2 : 8;

    if (rook_file == -1)
    {
      for (auto i = 0; i <= 7; i++)
      {
        if (board.get_piece_type(static_cast<Square>(i + Us * 56)) == Rook)
        {
          rook_file = i;// left outermost rook for side
          break;
        }
      }
      xfen = true;
    }

    pos->castle_rights |= CastleRights;
    castle_rights_mask[flip[Them][rook_file]] -= ooo_allowed_mask[Us];
    castle_rights_mask[flip[Them][file_of(board.king_square[Us])]] -= ooo_allowed_mask[Us];
    rook_castles_from[flip[Them][c1]] = flip[Them][rook_file];
    ooo_king_from[Us]                 = board.king_square[Us];

    if (file_of(board.king_square[Us]) != 4 || rook_file != 0)
      chess960 = true;
  }

  static Color get_side_to_move(const char **p) {
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

  static bool get_delimiter(const char **p) {
    if (**p != ' ')
      return false;

    (*p)++;
    return true;
  }

  void copy(Game *other) {
    board    = other->board;
    chess960 = other->chess960;
    xfen     = other->xfen;

    pos += other->pos - other->position_list;

    for (auto i = 0; i < 2000; i++)
    {
      position_list[i]       = other->position_list[i];
      position_list[i].board = &board;
    }
  }

  const char *move_to_string(const uint32_t m, char *buf) const {
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
        sprintf(&buf[strlen(buf)], "%s", pieceToString(move_promoted(m) & 7, tmp3));
    }
    return buf;
  }

  void print_moves() const {
    auto i = 0;
    char buf[12];

    while (const MoveData *m = pos->next_move())
    {
      printf("%d. ", i++ + 1);
      printf("%s", move_to_string(m->move, buf));
      printf("   %d\n", m->score);
    }
  }

public:
  Position *position_list;
  Position *pos;
  Board board;
  bool chess960;
  bool xfen;

  static constexpr std::string_view kStartPosition = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
};
