/*
  Feliscatus, a UCI chess playing engine derived from Tomcat 1.0 (Bobcat 8.0)
  Copyright (C) 2008-2016 Gunnar Harms (Bobcat author)
  Copyright (C) 2017      FireFather (Tomcat author)
  Copyright (C) 2020-2022 Rudy Alex Kohn

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

#include "perft.hpp"
#include "board.hpp"
#include "moves.hpp"

namespace
{

template<MoveGenFlags Flags>
u64 p(Board *b, const int depth)
{
  if (depth == 0)
    return 1;

  auto ml = MoveList<Flags>(b);

  [[unlikely]]
  if (depth == 1)
    return ml.size();

  u64 nodes{};

  for (const auto m : ml)
  {
    [[unlikely]]
    if (!b->make_move(m, true, true))
      continue;

    nodes += p<Flags>(b, depth - 1);
    b->unmake_move();
  }

  return nodes;
}

}   // namespace

template<MoveGenFlags Flags = LEGALMOVES>
struct Perft final
{
  Perft() = delete;
  Perft(Board *board) : b(board)
  { }
  explicit Perft(Board *board, int flags);

  u64 perft(int depth) const;

  u64 perft_divide(int depth) const;

private:
  Board *b{};
};

template<MoveGenFlags Flags>
u64 Perft<Flags>::perft(const int depth) const
{
  std::size_t nps{};
  u64 total_nodes{};
  Stopwatch sw;

  for (auto i = 1; i <= depth; i++)
  {
    start(&sw);
    const u64 nodes     = p<Flags>(b, i);
    const TimeUnit time = elapsed_milliseconds(&sw) + 1;
    total_nodes += nodes;
    nps = nodes / time * 1000;
    fmt::print("depth {}: {} nodes, {} ms, {} nps\n", i, nodes, time, nps);
  }

  return total_nodes;
}

template<MoveGenFlags Flags>
u64 Perft<Flags>::perft_divide(const i32 depth) const
{
  fmt::print("depth: {}\n", depth);

  u64 nodes{};
  TimeUnit time{};
  Stopwatch sw;

  auto ml = MoveList<Flags>(b);

  for (const auto m : ml)
  {
    [[unlikely]]
    if (!b->make_move(m, true, true))
      continue;

    const u64 nodes_start = nodes;
    start(&sw);
    nodes += p<Flags>(b, depth - 1);
    time += elapsed_milliseconds(&sw);
    b->unmake_move();
    fmt::print("move {}: {} nodes\n", b->move_to_string(m), nodes - nodes_start);
  }

  const auto nps = nodes / (time + 1) * 1000;

  fmt::print("{} nodes, {} nps", nodes, nps);

  return nodes;
}

u64 perft::perft(Board *b, const int depth)
{
  return Perft(b).perft(depth);
}

u64 perft::divide(Board *b, const int depth)
{
  return Perft(b).perft_divide(depth);
}
