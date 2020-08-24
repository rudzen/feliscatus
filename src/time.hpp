/*
  Feliscatus, a UCI chess playing engine derived from Tomcat 1.0 (Bobcat 8.0)
  Copyright (C) 2008-2016 Gunnar Harms (Bobcat author)
  Copyright (C) 2017      FireFather (Tomcat author)
  Copyright (C) 2020      Rudy Alex Kohn

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

#pragma once

#include "stopwatch.hpp"
#include "miscellaneous.hpp"
#include "search_limits.hpp"
#include "position.hpp"

struct Time final
{
  void init(Color c, SearchLimits &limits);

  [[nodiscard]]
  bool time_up() const noexcept;

  [[nodiscard]]
  bool plenty_time() const noexcept;

  void ponder_hit() noexcept;

  [[nodiscard]]
  TimeUnit elapsed() const noexcept;

  [[nodiscard]]
  bool should_post_curr_move() noexcept;

  bool should_post_info() noexcept;

  [[nodiscard]]
  bool is_analysing() const noexcept
  {
    return limits.infinite | limits.ponder;
  }

  [[nodiscard]]
  bool is_fixed_depth() const noexcept
  {
    return limits.fixed_depth;
  }

  [[nodiscard]]
  int depth() const noexcept
  {
    return limits.depth;
  }

private:
  Stopwatch start_time{};
  double n_{};
  SearchLimits limits{};
  TimeUnit search_time{};
  std::chrono::milliseconds last_curr_post{};
  std::chrono::milliseconds last_post_info{};
};
