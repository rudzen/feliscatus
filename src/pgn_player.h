#pragma once

#include <iostream>
#include "pgn.h"
#include "game.h"

namespace pgn {

class PGNPlayer : public PGNFileReader {
public:
  explicit PGNPlayer([[maybe_unused]] bool check_legal = true) : PGNFileReader(), game_(new Game()) { }

  virtual ~PGNPlayer() { delete game_; }

  void read_pgn_game() override {
    game_->new_game(Game::kStartPosition.data());
    PGNFileReader::read_pgn_game();
  }

  void read_tag_pair() override {
    PGNFileReader::read_tag_pair();

    if (strieq(tag_name_, "FEN"))
      game_->set_fen(std::string(tag_value_).substr(1, strlen(tag_value_) - 2).c_str());
  }

  void read_san_move() override {
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
      switch (from_piece_)
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
        std::cout << "default [" << token_str << "]" << '\n';
        exit(0);
      }
      game_->pos->generate_moves(piece, bit(to_square_));
    } else
    {
      std::cout << "else" << '\n';
      exit(0);
    }
    auto promoted = side_to_move << 3;

    if (promoted_to != -1)
    {
      switch (promoted_to)
      {
      case 'N':
        promoted |= Knight;
        break;

      case 'B':
        promoted |= Bishop;
        break;

      case 'R':
        promoted |= Rook;
        break;

      case 'Q':
        promoted |= Queen;
        break;

      default:
        std::cout << "promoted_to error [" << token_str << "]" << std::endl;
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
      std::cout << "!found [" << token_str << "]\n";
      std::cout << "to_square_:" << to_square_ << '\n';
      std::cout << "piece:" << piece << '\n';
      std::cout << "from_file_:" << from_file_ << '\n';
      std::cout << "from_rank_:" << from_rank_ << '\n';
      std::cout << "pawn_move_:" << pawn_move_ << '\n';
      std::cout << "castle_move_:" << castle_move_ << '\n';
      std::cout << "side_to_move:" << side_to_move << '\n';
      std::cout << "pos->in_check:" << game_->pos->in_check << '\n';
      std::cout << "game_count_:" << game_count_ << '\n';
      game_->board.print();
      exit(0);
    }
  }

protected:
  Game *game_;
};
}// namespace pgn