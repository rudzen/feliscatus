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

#include "types.h"

#pragma pack(1)
struct HashEntry {
  uint32_t key;
  uint16_t age;// 7 bits left
  uint8_t depth;
  NodeType flags;// 5 bits left
  int16_t score;
  Move move;
  int16_t eval;

  [[nodiscard]]
  bool is_exact() const noexcept { return flags & EXACT; }

  [[nodiscard]]
  bool is_beta() const noexcept { return flags & BETA; }

  [[nodiscard]]
  bool is_alpha() const noexcept { return flags & ALPHA; }
};
#pragma pack()

struct HashTable final {

  ~HashTable();

  void init(uint64_t new_size_mb);

  void clear();

  void init_search();

  [[nodiscard]]
  HashEntry *find(Key key) const;

  HashEntry *insert(Key key, int depth, int score, NodeType type, Move move, int eval);

  [[nodiscard]]
  HashEntry *get_entry_to_replace(Key key, [[maybe_unused]] int depth) const;

  [[nodiscard]]
  int get_load() const;

  [[nodiscard]]
  int get_size_mb() const;

protected:
  HashEntry *table{};
  uint64_t mask{};
  uint64_t occupied{};
  uint64_t size_mb{};
  uint64_t size{};
  int age{};
};

inline void HashTable::init_search() {
  age++;
}

inline int HashTable::get_load() const {
  return static_cast<int>(static_cast<double>(occupied) / size * 1000);
}

inline int HashTable::get_size_mb() const {
  return static_cast<int>(size_mb);
}

inline HashTable TT;
