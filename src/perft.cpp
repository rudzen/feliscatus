#include <fmt/format.h>
#include "perft.h"
#include "stopwatch.h"
#include "game.h"
#include "position.h"

struct perft_result {
  uint64_t nodes{};
  uint32_t enpassants{};
  uint32_t castles{};
  uint32_t captures{};
  uint32_t promotions{};
};

Perft::Perft(Game *game, const int flags) : g(game), perft_flags(flags) {}

Perft::Perft(Game *game) : g(game), perft_flags(LEGALMOVES) {}

void Perft::perft(const int depth) const {
  std::size_t nps{};

  for (auto i = 1; i <= depth; i++)
  {
    perft_result result;
    Stopwatch sw;
    perft(i, result);
    const auto time = sw.elapsed_milliseconds();
    if (time > 0.0)
      nps = result.nodes / time * 1000;
    fmt::print("depth {}: {} nodes, {} ms, {} nps\n", i, result.nodes, time, nps);
  }
}

void Perft::perft_divide(const int depth) const {
  fmt::print("depth: {}\n", depth);

  perft_result result{};
  auto *pos = g->pos;
  char buf[12];
  double time{};
  double nps{};

  pos->generate_moves(nullptr, 0, perft_flags);

  while (const MoveData *move_data = pos->next_move())
  {
    const auto *const m = &move_data->move;

    if (!g->make_move(*m, perft_flags == 0, true))
      continue;

    const auto nodes_start = result.nodes;
    Stopwatch sw;
    perft(depth - 1, result);
    time += sw.elapsed_milliseconds();
    g->unmake_move();
    fmt::print("move {}: {} nodes\n", g->move_to_string(*m, buf), result.nodes - nodes_start);
  }

  if (time > 0.0)
    nps = result.nodes / time * 1000;

  fmt::print("{} nodes, {} nps", result.nodes, nps);
}

int Perft::perft(const int depth, perft_result &result) const {
  if (depth == 0)
  {
    result.nodes++;
    return 0;
  }

  auto *pos = g->pos;
  pos->generate_moves(nullptr, 0, perft_flags);

  if ((perft_flags & STAGES) == 0 && depth == 1)
    result.nodes += pos->move_count();
  else
  {
    while (const MoveData *move_data = pos->next_move())
    {
      const auto *m = &move_data->move;

      if (!g->make_move(*m, perft_flags & LEGALMOVES ? false : true, true))
        continue;

      perft(depth - 1, result);
      g->unmake_move();
    }
  }
  return 0;
}
