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

#include <array>
#include <vector>

#include "miscellaneous.hpp"
#include "types.hpp"

struct SearchLimits final {
  std::array<TimeUnit, COL_NB> time{};
  std::array<TimeUnit, COL_NB> inc{};
  TimeUnit movetime{};
  int movestogo{};
  int depth{};
  bool ponder{};
  bool infinite{};
  bool fixed_movetime{};
  bool fixed_depth{};
  std::vector<Move> search_moves{};

  void clear() {
    time.fill(0);
    inc.fill(0);
    movetime          = 0;
    movestogo = depth = 0;
    ponder = infinite = fixed_movetime = fixed_depth = false;
  }
};
