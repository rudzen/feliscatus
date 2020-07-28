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

#include "types.hpp"
#include "miscellaneous.hpp"

#pragma pack(1)
struct alignas(16) HashEntry final
{
  [[nodiscard]]
  bool is_exact() const noexcept
  {
    return f & EXACT;
  }

  [[nodiscard]]
  bool is_beta() const noexcept
  {
    return f & BETA;
  }

  [[nodiscard]]
  bool is_alpha() const noexcept
  {
    return f & ALPHA;
  }

  [[nodiscard]]
  uint8_t depth() const noexcept
  {
    return d;
  }

  [[nodiscard]]
  NodeType flags() const noexcept
  {
    return static_cast<NodeType>(f & 7);
  }

  [[nodiscard]]
  int16_t score() const noexcept
  {
    return s;
  }

  [[nodiscard]]
  int16_t eval() const noexcept
  {
    return e;
  }

  [[nodiscard]]
  Move move() const noexcept
  {
    return m;
  }

private:
  uint32_t k;   // key
  uint16_t a;   // age, 7 bits left
  uint8_t d;    // depth
  NodeType f;   // flags, 5 bits left
  int16_t s;    // score
  Move m;       // move
  int16_t e;    // eval

  friend class HashTable;
};
#pragma pack()

struct HashTable final
{
private:
  friend struct HashEntry;

  static constexpr int CacheLineSize      = 64;
  static constexpr std::size_t BucketSize = 4;

  using BucketArray = std::array<HashEntry, BucketSize>;

  // Just use a simple array for bucket
  struct Bucket final
  {
    BucketArray entry{};
  };

public:
  ~HashTable();
  constexpr HashTable()             = default;
  HashTable(const HashTable &other) = delete;
  HashTable(HashTable &&other)      = delete;
  HashTable &operator=(const HashTable &) = delete;
  HashTable &operator=(HashTable &&other) = delete;

  void init(uint64_t new_size_mb);

  void clear();

  void init_search();

  [[nodiscard]]
  HashEntry *first_entry(const Key key) const
  {
    return &table_[mul_hi64(key, bucket_count_)].entry[0];
  }

  [[nodiscard]]
  Bucket *find_bucket(const Key key) const
  {
    return &table_[mul_hi64(key, bucket_count_)];
  }

  [[nodiscard]]
  HashEntry *find(Key key) const;

  HashEntry *insert(Key key, int depth, int score, NodeType nt, Move m, int eval);

  [[nodiscard]]
  HashEntry *get_entry_to_replace(Key key, [[maybe_unused]] int depth) const;

  [[nodiscard]]
  int load() const;

  [[nodiscard]]
  int size_mb() const;

private:
  static_assert(CacheLineSize % sizeof(Bucket) == 0, "Bucket size incorrect");

  Bucket *table_{};
  void *mem_{};

  std::size_t bucket_count_{};
  std::size_t fullness_element_{};
  uint64_t occupied_{};
  uint64_t size_mb_{};
  uint64_t size_{};
  int age_{};
};

inline void HashTable::init_search()
{
  age_++;
}

inline int HashTable::load() const
{
  return static_cast<int>(static_cast<double>(occupied_) / fullness_element_ * 1000);
}

inline int HashTable::size_mb() const
{
  return static_cast<int>(size_mb_);
}

constinit inline HashTable TT;
