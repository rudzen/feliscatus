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
#include "stopwatch.hpp"
#include "board.hpp"
#include "moves.hpp"

namespace
{

std::uint64_t p(Board *b, const int depth)
{
  if (depth == 0)
    return 1;

  auto ml = MoveList<LEGALMOVES>(b);

  [[unlikely]]
  if (depth == 1)
    return ml.size();

  std::uint64_t nodes{};

  for (const auto m : ml)
  {
    [[unlikely]] if (!b->make_move(m, true, true)) continue;

    nodes += p(b, depth - 1);
    b->unmake_move();
  }

  return nodes;
}

}   // namespace

struct Perft final
{
  Perft() = delete;
  explicit Perft(Board *board);
  explicit Perft(Board *board, int flags);

  std::uint64_t perft(int depth) const;

  std::uint64_t perft_divide(int depth) const;

private:
  Board *b{};
  int perft_flags{};
};

Perft::Perft(Board *board, const int flags) : b(board), perft_flags(flags)
{ }

Perft::Perft(Board *board) : Perft(board, LEGALMOVES)
{ }

std::uint64_t Perft::perft(const int depth) const
{
  std::size_t nps{};

  std::uint64_t total_nodes{};

  for (auto i = 1; i <= depth; i++)
  {
    Stopwatch sw;
    const auto nodes = p(b, i);
    total_nodes += nodes;
    const auto time = sw.elapsed_milliseconds() + 1;
    nps             = nodes / time * 1000;
    fmt::print("depth {}: {} nodes, {} ms, {} nps\n", i, nodes, time, nps);
  }

  return total_nodes;
}

std::uint64_t Perft::perft_divide(const int depth) const
{
  fmt::print("depth: {}\n", depth);

  std::uint64_t nodes{};
  TimeUnit time{};

  auto ml = MoveList<LEGALMOVES>(b);

  for (const auto m : ml)
  {
    [[unlikely]]
    if (!b->make_move(m, true, true))
      continue;

    const auto nodes_start = nodes;
    Stopwatch sw;
    nodes += p(b, depth - 1);
    time += sw.elapsed_milliseconds();
    b->unmake_move();
    fmt::print("move {}: {} nodes\n", b->move_to_string(m), nodes - nodes_start);
  }

  const auto nps = nodes / (time + 1) * 1000;

  fmt::print("{} nodes, {} nps", nodes, nps);

  return nodes;
}

std::uint64_t perft::perft(Board *b, const int depth, const int flags)
{
  return Perft(b, flags).perft(depth);
}

std::uint64_t perft::divide(Board *b, const int depth, const int flags)
{
  return Perft(b, flags).perft_divide(depth);
}
