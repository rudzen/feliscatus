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

#include <algorithm>
#include "timemanager.h"
#include "util.h"

namespace {

constexpr TimeUnit time_reserve = 72;

constexpr TimeUnit curr_move_post_limit = 5000;

}

void TimeManager::init(const Color c, SearchLimits &search_limits) {

  limits = search_limits;

  start_time.start();
  last_curr_post = start_time.elapsed_milliseconds();
  if (limits.fixed_movetime)
    search_time = 950 * limits.movetime / 1000;
  else
  {
    const auto moves_left = util::in_between<1, 30>(limits.movestogo) ? limits.movestogo : 30;
    const auto time_left = limits.time[c];
    const auto time_inc  = limits.inc[c];

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

bool TimeManager::time_up() const noexcept {
  return start_time.elapsed_milliseconds() > search_time;
}

bool TimeManager::plenty_time() const noexcept {
  return search_time < start_time.elapsed_milliseconds() * n_;
}

void TimeManager::ponder_hit() noexcept {
  search_time += start_time.elapsed_milliseconds();
}

TimeUnit TimeManager::elapsed() const noexcept {
  return start_time.elapsed_milliseconds();
}

bool TimeManager::should_post_curr_move() noexcept {
  const auto ok = start_time.elapsed_milliseconds() + last_curr_post > curr_move_post_limit;
  if (ok)
    last_curr_post -= curr_move_post_limit;
  return ok;
}
