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

std::chrono::milliseconds since_epoch()
{
  return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
}

}   // namespace

void start(Stopwatch *sw)
{
  sw->start_time = std::chrono::system_clock::now();
  sw->running    = true;
}

void stop(Stopwatch *sw)
{
  sw->start_time = std::chrono::system_clock::now();
  sw->running    = false;
}

TimeUnit elapsed_milliseconds(const Stopwatch *sw)
{
  const std::chrono::time_point<std::chrono::system_clock> end_time =
    sw->running ? std::chrono::system_clock::now() : sw->end_time;
  return std::chrono::duration_cast<std::chrono::milliseconds>(end_time - sw->start_time).count();
}
TimeUnit elapsed_microseconds(const Stopwatch *sw)
{
  const auto end_time = sw->running ? std::chrono::system_clock::now() : sw->end_time;
  return std::chrono::duration_cast<std::chrono::microseconds>(end_time - sw->start_time).count();
}

TimeUnit elapsed_seconds(const Stopwatch *sw)
{
  const auto end_time = sw->running ? std::chrono::system_clock::now() : sw->end_time;
  return std::chrono::duration_cast<std::chrono::seconds>(end_time - sw->start_time).count();
}

void init_time(Time *time, const Color c, const SearchLimits *limits)
{
  time->last_curr_post = time->last_post_info = since_epoch();

  start(&time->start_time);

  [[unlikely]]
  if (limits->fixed_movetime)
    time->search_time = 950 * limits->movetime / 1000;
  else
  {
    const i32 moves_left     = util::inBetween<1, 30>(limits->movestogo) ? limits->movestogo : 30;
    const TimeUnit time_left = limits->time[c];
    const TimeUnit time_inc  = limits->inc[c];

    [[unlikely]]
    if (time_inc == 0 && time_left < 1000)
    {
      time->search_time = time_left / (moves_left * 2);
      time->n          = 1;
    } else
    {
      time->search_time = 2 * (time_left / (moves_left + 1) + time_inc);
      time->n          = 2.5;
    }


    time->search_time = std::max<TimeUnit>(0, std::min<TimeUnit>(time->search_time, time_left - time_reserve));
  }
}

bool is_time_up(const Time *time)
{
  return elapsed_milliseconds(&time->start_time) > time->search_time;
}

bool has_plenty_time(const Time *time)
{
  return time->search_time < elapsed_milliseconds(&time->start_time) * time->n;
}

void ponder_hit(Time *time)
{
  time->search_time += elapsed_milliseconds(&time->start_time);
}

TimeUnit elapsed(const Time *time)
{
  return elapsed_milliseconds(&time->start_time);
}

bool should_post_current_move(Time *time)
{
  const std::chrono::milliseconds now = since_epoch();
  const bool can_post                 = now - time->last_curr_post > curr_move_post_limit;

  if (can_post)
    time->last_curr_post = now;
  return can_post;
}

bool should_post_info(Time *time)
{
  const std::chrono::milliseconds now = since_epoch();
  const bool can_post                 = now - time->last_post_info > last_post_info_span;

  if (can_post)
    time->last_post_info = now;
  return can_post;
}