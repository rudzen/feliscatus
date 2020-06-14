#include "pgn_player.h"
#include <fmt/format.h>
#include "bitboard.h"
#include "piece.h"
#include "util.h"
#include "game.h"
#include "position.h"

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

}

pgn::PGNPlayer::PGNPlayer([[maybe_unused]] bool check_legal)
  : PGNFileReader(), game_(new Game()) {
}

pgn::PGNPlayer::~PGNPlayer() { delete game_; }

void pgn::PGNPlayer::read_pgn_game() {
  game_->new_game(Game::kStartPosition.data());
  pgn::PGNFileReader::read_pgn_game();
}

void pgn::PGNPlayer::read_tag_pair() {
  PGNFileReader::read_tag_pair();

  if (util::strieq(tag_name_, "FEN"))
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
      fmt::print("default [{}]\n", token_str);
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
      fmt::print("promoted_to error [{}]\n", token_str);
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
    fmt::print("game_count_: {}", game_count_);
    game_->board.print();
    exit(0);
  }
}
