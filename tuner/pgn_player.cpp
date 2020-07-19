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

#include <fmt/format.h>

#include "pgn_player.hpp"
#include "../src/bitboard.hpp"
#include "../src/util.hpp"
#include "../src/board.hpp"
#include "../src/position.hpp"
#include "../src/tpool.hpp"
#include "../src/moves.hpp"

namespace {

constexpr auto detect_piece = [](const int from) {
  switch (from)
  {
  case 'N':
    return KNIGHT;
  case 'B':
    return BISHOP;
  case 'R':
    return ROOK;
  case 'Q':
    return QUEEN;
  case 'K':
    return KING;
  default:
    return NO_PT;
  };
};

bool strieq(const char *s1, const char *s2) {
  if (std::strlen(s1) != std::strlen(s2))
    return false;

  for (std::size_t i = 0; i < std::strlen(s1); i++)
    if (::tolower(*(s1 + i)) != ::tolower(*(s2 + i)))
      return false;

  return true;
}

}

pgn::PGNPlayer::PGNPlayer([[maybe_unused]] bool check_legal)
  : PGNFileReader(), b(std::make_unique<Board>()) {
}

void pgn::PGNPlayer::read_pgn_game() {
  b->new_game(pool.main());
  pgn::PGNFileReader::read_pgn_game();
}

void pgn::PGNPlayer::read_tag_pair() {
  PGNFileReader::read_tag_pair();

  if (strieq(tag_name_, "FEN"))
  {
    const auto fen = std::string(tag_value_).substr(1, strlen(tag_value_) - 2);
    b->set_fen(fen, pool.main());
  }
}

void pgn::PGNPlayer::read_san_move() {
  PGNFileReader::read_san_move();

  Piece piece{NO_PIECE};

  auto mg = Moves<true>(b.get());

  if (pawn_move_)
  {
    piece = make_piece(PAWN, side_to_move);
    mg.generate_pawn_moves(capture_, bit(to_square_), side_to_move);
  } else if (castle_move_)
  {
    piece = make_piece(KING, side_to_move);
    mg.generate_moves();
  } else if (piece_move_)
  {
    const auto pt = detect_piece(from_piece_);
    if (pt == NO_PT)
    {
      fmt::print("default [{}]\n", std::string(token_str));
      exit(0);
    }
    piece = make_piece(pt, side_to_move);
    if (side_to_move == WHITE)
      mg.generate_moves<WHITE>(pt, bit(to_square_));
    else
      mg.generate_moves<BLACK>(pt, bit(to_square_));
  } else
  {
    fmt::print("else\n");
    exit(0);
  }

  Piece promoted{NO_PIECE};

  if (promoted_to != -1)
  {
    const auto pt = detect_piece(promoted_to);
    if (pt == NO_PT)
    {
      fmt::print("promoted_to error [{}]\n", std::string(token_str));
      exit(0);
    }
    promoted = make_piece(pt, side_to_move);
  }

  auto found            = false;

  while (auto *const move_data = mg.next_move())
  {
    const auto m = move_data->move;
    if (move_piece(m) != piece || move_to(m) != to_square_ || (promoted != NO_PIECE && move_promoted(m) != promoted) || (capture_ && !is_capture(m))
        || (from_file_ != -1 && file_of(move_from(m)) != from_file_) || (from_rank_ != -1 && rank_of(move_from(m)) != from_rank_))
      continue;

    if (!b->make_move(m, true, true))
      continue;

    found = true;
    break;
  }

  if (!found)
  {
    fmt::print("!found [{}]\n", token_str);
    fmt::print("to_square_: {}\n", to_square_);
    fmt::print("piece: {}\n", piece);
    fmt::print("from_file_: {}\n", from_file_);
    fmt::print("from_rank_: {}\n", from_rank_);
    fmt::print("pawn_move_: {}\n", pawn_move_);
    fmt::print("castle_move_: {}\n", castle_move_);
    fmt::print("side_to_move: {}\n", side_to_move);
    fmt::print("pos->in_check: {}\n", b->in_check());
    fmt::print("game_count_: {}\n", game_count_);
    b->print();
    exit(0);
  }
}
