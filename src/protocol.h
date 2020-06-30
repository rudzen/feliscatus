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

#include <cstdint>
#include <string_view>
#include <array>
#include <sstream>

#include <fmt/format.h>

#include "stopwatch.h"
#include "types.h"

enum NodeType : uint8_t;
enum Move : uint32_t;
class Game;
struct PVEntry;

struct SearchLimits {
  std::array<TimeUnit, COL_NB> time{};
  std::array<TimeUnit, COL_NB> inc{};
  TimeUnit movetime{};
  int movestogo{};
  int depth{};
  bool ponder{};
  bool infinite{};
  bool fixed_movetime{};
  bool fixed_depth{};

  void clear() {
    time.fill(0);
    inc.fill(0);
    movetime          = 0;
    movestogo = depth = 0;
    ponder = infinite = fixed_movetime = fixed_depth = false;
  }
};

struct ProtocolListener {
  virtual ~ProtocolListener()                                            = default;
  virtual int new_game()                                                 = 0;
  virtual int set_fen(std::string_view fen)                              = 0;
  virtual int go()                             = 0;
  virtual void ponder_hit()                                              = 0;
  virtual void stop()                                                    = 0;
  virtual bool set_option(std::string_view name, std::string_view value) = 0;
};
