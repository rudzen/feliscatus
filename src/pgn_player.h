#pragma once

#include "pgn.h"

class Game;

namespace pgn {

struct PGNPlayer : public PGNFileReader {

  explicit PGNPlayer(bool check_legal = true);

  virtual ~PGNPlayer();

  void read_pgn_game() override;

  void read_tag_pair() override;

  void read_san_move() override;

protected:
  Game *game_;
};
}// namespace pgn