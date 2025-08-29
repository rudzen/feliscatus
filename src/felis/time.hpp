// Copyright (C) 2008-2016 Gunnar Harms (Bobcat author)
// Copyright (C) 2017      FireFather (Tomcat author)
// Copyright (C) 2020-2025 Rudy Alex Kohn
// See end of file for extended copyright information.

#pragma once

#include <felis/miscellaneous.hpp>
#include <felis/search_limits.hpp>
#include <felis/position.hpp>

struct Stopwatch
{
  std::chrono::time_point<std::chrono::system_clock> start_time;
  std::chrono::time_point<std::chrono::system_clock> end_time;
  std::chrono::time_point<std::chrono::system_clock> last_curr_info;
  bool running;
};

struct Time
{
  Stopwatch start_time;
  TimeUnit search_time;
  std::chrono::milliseconds last_curr_post;
  std::chrono::milliseconds last_post_info;
  r64 n;
};

void start(Stopwatch *sw);

void stop(Stopwatch *sw);

TimeUnit elapsed_milliseconds(const Stopwatch *sw);

TimeUnit elapsed_microseconds(const Stopwatch *sw);

TimeUnit elapsed_seconds(const Stopwatch *sw);

void init_time(Time *time, Color c, const SearchLimits *limits);

bool is_time_up(const Time * time);

bool has_plenty_time(const Time * time);

void ponder_hit(Time* time);

TimeUnit elapsed(const Time * time);

bool should_post_current_move(Time* time);

bool should_post_info(Time* time);

// Feliscatus, a UCI chess playing engine derived from Tomcat 1.0 (Bobcat 8.0)
// Copyright (C) 2008-2016 Gunnar Harms (Bobcat author)
// Copyright (C) 2017      FireFather (Tomcat author)
// Copyright (C) 2020-2022 Rudy Alex Kohn
//
// Feliscatus is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Feliscatus is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Feliscatus.  If not, see <http://www.gnu.org/licenses/>.