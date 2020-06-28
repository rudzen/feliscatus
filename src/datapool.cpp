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

#include <cstring>
#include <numeric>

#include "datapool.h"

Data::Data(const std::size_t data_index) : index(data_index) {}

void Data::clear_data() {
  std::memset(history_scores, 0, sizeof history_scores);
  std::memset(counter_moves, 0, sizeof counter_moves);
  node_count.store(0);
}

void DataPool::set(const std::size_t v) {

  while (!empty())
    pop_back();

  if (v > 0)
  {
    emplace_back(std::make_unique<MainData>(0));

    while (size() < v)
      emplace_back(std::make_unique<Data>(size()));

    clear_data();
  }
}

void DataPool::clear_data() {
  for (auto &w: *this)
    w->clear_data();
}

uint64_t DataPool::node_count() const {
  const auto accumulator = [](const uint64_t r, const std::unique_ptr<Data> &d) { return r + d->node_count.load(std::memory_order_relaxed); };
  return std::accumulate(cbegin(), cend(), 0ull, accumulator);
}
