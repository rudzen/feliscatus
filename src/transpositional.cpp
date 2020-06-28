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
#include <cstdlib>
#include <fmt/format.h>
#include "transpositional.h"

namespace {

constexpr int NUMBER_SLOTS = 4;

constexpr uint32_t key32(const Key key) {
  return key >> 32;
}

}

void HashTable::init(const uint64_t new_size_mb) {
  if (new_size_mb == size_mb)
    return;

  size_mb = new_size_mb;
  size    = 1024 * 1024 * new_size_mb / sizeof(HashEntry);
  mask    = size - 1;
  size += NUMBER_SLOTS - 1;
  delete[] table;
  table = new HashEntry[size];
  clear();
}

void HashTable::clear() {
  std::memset(table, 0, size * sizeof(HashEntry));
  occupied = 0;
  age      = 0;
}

HashEntry *HashTable::find(const Key key) const {
  auto *transp   = table + (key & mask);
  const auto k32 = key32(key);
  for (auto i = 0; i < NUMBER_SLOTS; i++, transp++)
    if (transp->key == k32 && transp->flags)
      return transp;
  return nullptr;
}

HashEntry *HashTable::insert(const Key key, const int depth, const int score, const NodeType type, const Move move, const int eval) {
  auto *transp = get_entry_to_replace(key, depth);

  if (transp->flags == 0)
    occupied++;

  const auto k32 = key32(key);

  if (transp->key != k32 || move != MOVE_NONE)
    transp->move = move;
  transp->key   = k32;
  transp->score = static_cast<int16_t>(score);
  transp->depth = static_cast<uint8_t>(depth);
  transp->flags = type;
  transp->age   = static_cast<uint16_t>(age);
  transp->eval  = static_cast<int16_t>(eval);
  return transp;
}

HashEntry *HashTable::get_entry_to_replace(const Key key, [[maybe_unused]] const int depth) const {
  auto *transp   = table + (key & mask);
  const auto k32 = key32(key);

  if (transp->flags == 0 || transp->key == k32)
    return transp;

  constexpr auto replacement_score = [](const HashEntry *e) {
    return (e->age << 9) + e->depth;
  };

  auto *replace      = transp++;
  auto replace_score = replacement_score(replace);

  for (auto i = 1; i < NUMBER_SLOTS; i++, transp++)
  {
    if (transp->flags == Void || transp->key == k32)
      return transp;

    const auto score = replacement_score(transp);

    if (score < replace_score)
    {
      replace_score = score;
      replace       = transp;
    }
  }
  return replace;
}
