#pragma once

#include <cstdint>

class Game;
struct perft_result;

struct Perft final {
  Perft() = delete;
  explicit Perft(Game *game);
  explicit Perft(Game *game, int flags);

  void perft(int depth) const;

  void perft_divide(int depth) const;

private:
  int perft(int depth, perft_result &result) const;

  Game *g{};
  int perft_flags{};
};