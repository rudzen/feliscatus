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

#include "pgn_player.h"
#include "../src/bitboard.h"
#include "../src/util.h"
#include "../src/game.h"
#include "../src/position.h"

namespace {

constexpr auto detect_piece = [](const int from, int &piece) {
  switch (from)
  {
  case 'N':
    piece |= Knight;
    break;

  case 'B':
    piece |= Bishop;
    break;

  case 'R':
    piece |= Rook;
    break;

  case 'Q':
    piece |= Queen;
    break;

  case 'K':
    piece |= King;
    break;
    default:
    return false;
  };

  return true;
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
  : PGNFileReader(), game_(std::make_unique<Game>()) {
}

void pgn::PGNPlayer::read_pgn_game() {
  game_->new_game(Game::kStartPosition);
  pgn::PGNFileReader::read_pgn_game();
}

void pgn::PGNPlayer::read_tag_pair() {
  PGNFileReader::read_tag_pair();

  if (strieq(tag_name_, "FEN"))
    game_->set_fen(std::string(tag_value_).substr(1, strlen(tag_value_) - 2).c_str());
}

void pgn::PGNPlayer::read_san_move() {
  PGNFileReader::read_san_move();

  auto piece = side_to_move << 3;

  if (pawn_move_)
  {
    piece |= Pawn;
    game_->pos->generate_pawn_moves(capture_, bit(to_square_));
  } else if (castle_move_)
  {
    piece |= King;
    game_->pos->generate_moves();
  } else if (piece_move_)
  {
    if (!detect_piece(from_piece_, piece))
    {
      fmt::print("default [{}]\n", std::string(token_str));
      exit(0);
    }
    game_->pos->generate_moves(piece, bit(to_square_));
  } else
  {
    fmt::print("else\n");
    exit(0);
  }
  auto promoted = side_to_move << 3;

  if (promoted_to != -1)
  {
    if (!detect_piece(promoted_to, promoted))
    {
      fmt::print("promoted_to error [{}]\n", std::string(token_str));
      exit(0);
    }
  }

  auto found            = false;
  const auto move_count = game_->pos->move_count();

  for (auto i = 0; i < move_count; ++i)
  {
    const auto m = game_->pos->move_list[i].move;

    if (move_piece(m) != piece || move_to(m) != to_square_ || (promoted_to != -1 && move_promoted(m) != promoted) || (capture_ && !is_capture(m))
        || (from_file_ != -1 && file_of(move_from(m)) != from_file_) || (from_rank_ != -1 && rank_of(move_from(m)) != from_rank_))
      continue;

    if (!game_->make_move(m, true, true))
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
    fmt::print("pos->in_check: {}\n", game_->pos->in_check);
    fmt::print("game_count_: {}\n", game_count_);
    game_->board.print();
    exit(0);
  }
}
