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
#include <vector>
#include <memory>
#include <atomic>
#include "types.h"
#include "pawnhashtable.h"
#include "pv_entry.h"
#include "timemanager.h"
#include "search_limits.h"

struct Data {

  explicit Data(std::size_t data_index);

  void clear_data();

  PawnHashTable pawn_hash{};
  int history_scores[16][64]{};
  Move counter_moves[16][64]{};
  std::array<std::array<PVEntry, MAXDEPTH>, MAXDEPTH> pv{};
  std::array<int, MAXDEPTH> pv_length{};

  std::atomic_uint64_t node_count;

private:
  std::size_t index;

};

struct MainData : Data {

  using Data::Data;
};

struct DataPool : std::vector<std::unique_ptr<Data>> {

  void set(std::size_t v);

  [[nodiscard]]
  MainData *main() const { return static_cast<MainData *>(front().get()); }

  void clear_data();

  [[nodiscard]]
  uint64_t node_count() const;

  TimeManager time{};

  SearchLimits limits{};
};

// global data object
inline DataPool Pool{};