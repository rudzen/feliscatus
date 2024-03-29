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

#include <algorithm>
#include <chrono>

#include "time.hpp"

namespace
{

constexpr TimeUnit time_reserve = 72;
constexpr std::chrono::milliseconds curr_move_post_limit(5000);
constexpr std::chrono::milliseconds last_post_info_span(1000);

inline std::chrono::milliseconds since_epoch()
{
  return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
}

}   // namespace

void Time::init(const Color c, SearchLimits &limits)
{

  last_curr_post = last_post_info = since_epoch();

  start_time.start();
  [[unlikely]]
  if (limits.fixed_movetime)
    search_time = 950 * limits.movetime / 1000;
  else
  {
    const auto moves_left = util::in_between<1, 30>(limits.movestogo) ? limits.movestogo : 30;
    const auto time_left  = limits.time[c];
    const auto time_inc   = limits.inc[c];

    [[unlikely]]
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

bool Time::time_up() const noexcept
{
  return start_time.elapsed_milliseconds() > search_time;
}

bool Time::plenty_time() const noexcept
{
  return search_time < start_time.elapsed_milliseconds() * n_;
}

void Time::ponder_hit() noexcept
{
  search_time += start_time.elapsed_milliseconds();
}

TimeUnit Time::elapsed() const noexcept
{
  return start_time.elapsed_milliseconds();
}

bool Time::should_post_curr_move() noexcept
{
  const auto now = since_epoch();
  const auto can_post = now - last_curr_post > curr_move_post_limit;
  if (can_post)
    last_curr_post = now;
  return can_post;
}

bool Time::should_post_info() noexcept
{
  const auto now = since_epoch();
  const auto can_post = now - last_post_info > last_post_info_span;
  if (can_post)
    last_post_info = now;
  return can_post;
}