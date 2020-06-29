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

#include <algorithm>
#include "stopwatch.h"
#include "miscellaneous.h"
#include "protocol.h"
#include "position.h"
#include "util.h"

struct TimeManager final {

  void init(Color side_to_move, SearchLimits *limits);

  [[nodiscard]]
  bool is_analysing() const noexcept { return search_limits->infinite | search_limits->ponder; }

  [[nodiscard]]
  bool is_fixed_depth() const noexcept { return search_limits->fixed_depth; }

  [[nodiscard]]
  int get_depth() const noexcept { return search_limits->depth; }

  double n_{};
  Stopwatch start_time{};
  TimeUnit search_time{};
  TimeUnit time_left{};
  TimeUnit time_inc{};
  SearchLimits *search_limits;
};

inline void TimeManager::init(const Color side_to_move, SearchLimits *limits) {
  constexpr auto time_reserve = 72;

  search_limits = limits;

  start_time.start();

  if (limits->fixed_movetime)
    search_time = 950 * limits->movetime / 1000;
  else
  {
    auto moves_left = util::in_between<1, 30>(limits->movestogo) ? limits->movestogo : 30;

    time_left = limits->time[side_to_move];
    time_inc  = limits->inc[side_to_move];

    if (time_inc == 0 && time_left < 1000)
    {
      search_time = time_left / (moves_left * 2);
      n_          = 1;
    } else
    {
      search_time = 2 * (time_left / (moves_left + 1) + time_inc);
      n_          = 2.5;
    }
    search_time = std::max<TimeUnit>(0, std::min<int>(search_time, time_left - time_reserve));
  }
}