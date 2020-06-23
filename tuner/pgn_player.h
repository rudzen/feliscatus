#pragma once

#include <memory>
#include "pgn.h"

class Game;

namespace pgn {

struct PGNPlayer : PGNFileReader {

  explicit PGNPlayer(bool check_legal = true);

  virtual ~PGNPlayer() = default;

  void read_pgn_game() override;

  void read_tag_pair() override;

  void read_san_move() override;

protected:
  std::unique_ptr<Game> game_;
};
}// namespace pgn