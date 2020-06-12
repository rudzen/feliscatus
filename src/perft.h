#pragma once

#include <cstdint>
#include <fmt/format-inl.h>

struct perft_result {
  perft_result() { nodes = enpassants = castles = captures = promotions = 0; }
  uint64_t nodes;
  uint32_t enpassants;
  uint32_t castles;
  uint32_t captures;
  uint32_t promotions;
};

class Perft {
public:
  Perft(Game *game, const int flags = LEGALMOVES) {
    this->game  = game;
    this->flags = flags;
  }

  void perft(const int depth) const {
    std::size_t nps = 0;

    for (auto i = 1; i <= depth; i++)
    {
      perft_result result;
      Stopwatch sw;
      perft(i, result);
      const auto time = sw.millisElapsed() / static_cast<double>(1000);
      if (time)
        nps = result.nodes / time;
      fmt::print("depth {}: {} nodes, {} secs, {} nps\n", i, result.nodes, time, nps);
    }
  }

  void perft_divide(const int depth) const {
    fmt::print("depth: {}\n", depth);

    perft_result result;
    auto *pos = game->pos;
    char buf[12];
    double time = 0;
    double nps  = 0;

    pos->generate_moves(nullptr, 0, flags);

    while (const MoveData *move_data = pos->next_move())
    {
      const auto *const m = &move_data->move;

      if (!game->make_move(*m, flags == 0 ? true : false, true))
        continue;

      const auto nodes_start = result.nodes;
      Stopwatch sw;
      perft(depth - 1, result);
      time += sw.millisElapsed() / static_cast<double>(1000);
      game->unmake_move();
      fmt::print("move {}: {} nodes\n", game->move_to_string(*m, buf), result.nodes - nodes_start);
    }
    if (time)
      nps = result.nodes / time;

    fmt::print("{} nodes, {} nps", result.nodes, nps);
  }

private:
  double total_time;

  int perft(const int depth, perft_result &result) const {
    if (depth == 0)
    {
      result.nodes++;
      return 0;
    }
    auto *pos = game->pos;
    pos->generate_moves(nullptr, 0, flags);

    if ((flags & STAGES) == 0 && depth == 1)
      result.nodes += pos->move_count();
    else
    {
      while (const MoveData *move_data = pos->next_move())
      {
        const auto *m = &move_data->move;

        if (!game->make_move(*m, (flags & LEGALMOVES) ? false : true, true))
          continue;

        perft(depth - 1, result);
        game->unmake_move();
      }
    }
    return 0;
  }

  Game *game;
  Search *search{};
  ProtocolListener *app{};
  int flags;
};