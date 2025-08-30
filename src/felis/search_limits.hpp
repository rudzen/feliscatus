// Copyright (C) 2008-2016 Gunnar Harms (Bobcat author)
// Copyright (C) 2017      FireFather (Tomcat author)
// Copyright (C) 2020-2025 Rudy Alex Kohn
// See end of file for extended copyright information.

#pragma once

#include <cstring>
#include <felis/miscellaneous.hpp>
#include <felis/types.hpp>

struct SearchLimits final {
  TimeUnit time[2];
  TimeUnit inc[2];
  TimeUnit movetime;
  i32 movestogo;
  i32 depth;
  bool ponder;
  bool infinite;
  bool fixed_movetime;
  bool fixed_depth;
  Move* search_moves;
  u16 search_moves_count;
};

inline void ClearSearchLimits(SearchLimits* limits) {
  std::memset(limits, 0, sizeof(SearchLimits) + sizeof(Move) * MAX_MOVES);
}

// Feliscatus, a UCI chess playing engine derived from Tomcat 1.0 (Bobcat 8.0)
// Copyright (C) 2008-2016 Gunnar Harms (Bobcat author)
// Copyright (C) 2017      FireFather (Tomcat author)
// Copyright (C) 2020-2025 Rudy Alex Kohn
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