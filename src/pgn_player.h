#pragma once

#include <iostream>
#include "pgn.h"
#include "game.h"

using namespace std;

namespace pgn {

class PGNPlayer : public PGNFileReader {
public:
  PGNPlayer([[maybe_unused]] bool check_legal = true) : PGNFileReader(), game_(nullptr) { game_ = new Game(); }

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

    int piece = (side_to_move << 3);

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
        cout << "default [" << token_str << "]" << endl;
        exit(0);
      }
      game_->pos->generate_moves(piece, bit(to_square_));
    } else
    {
      cout << "else" << endl;
      exit(0);
    }
    int promoted = (side_to_move << 3);

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
        cout << "promoted_to error [" << token_str << "]" << endl;
        exit(0);
      }
    }
    bool found     = false;
    const int move_count = game_->pos->move_count();

    for (int i = 0; i < move_count; ++i)
    {
      const uint32_t m = game_->pos->move_list[i].move;

      if (move_piece(m) != piece || move_to(m) != to_square_ || (promoted_to != -1 && move_promoted(m) != promoted) || (capture_ && !is_capture(m))
          || (from_file_ != -1 && file_of(move_from(m)) != from_file_) || (from_rank_ != -1 && rank_of(move_from(m)) != from_rank_))
      {
        continue;
      }

      if (!game_->make_move(m, true, true))
      {
        continue;
      }
      found = true;
      break;
    }

    if (!found)
    {
      cout << "!found [" << token_str << "]" << endl;
      cout << "to_square_:" << to_square_ << endl;
      cout << "piece:" << piece << endl;
      cout << "from_file_:" << from_file_ << endl;
      cout << "from_rank_:" << from_rank_ << endl;
      cout << "pawn_move_:" << pawn_move_ << endl;
      cout << "castle_move_:" << castle_move_ << endl;
      cout << "side_to_move:" << side_to_move << endl;
      cout << "pos->in_check:" << game_->pos->in_check << endl;
      cout << "game_count_:" << game_count_ << endl;
      game_->board.print();
      exit(0);
    }
  }

protected:
  Game *game_;
};
}// namespace pgn