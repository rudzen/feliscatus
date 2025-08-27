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

#pragma once

#include <cstdint>

#include "types.hpp"
#include "miscellaneous.hpp"

struct PVEntry;

#pragma pack(1)
struct alignas(CacheLineSize / 4) HashEntry final
{
  [[nodiscard]]
  bool isExact() const noexcept
  {
    return f & EXACT;
  }

  [[nodiscard]]
  bool isBeta() const noexcept
  {
    return f & BETA;
  }

  [[nodiscard]]
  bool isAlpha() const noexcept
  {
    return f & ALPHA;
  }

  [[nodiscard]]
  std::uint8_t depth() const noexcept
  {
    return d;
  }

  [[nodiscard]]
  NodeType flags() const noexcept
  {
    return static_cast<NodeType>(f & 7);
  }

  [[nodiscard]]
  std::int16_t score() const noexcept
  {
    return s;
  }

  [[nodiscard]]
  std::int16_t eval() const noexcept
  {
    return e;
  }

  [[nodiscard]]
  Move move() const noexcept
  {
    return m;
  }

private:
  std::uint32_t k;   // key
  std::uint16_t a;   // age, 7 bits left
  std::uint8_t d;    // depth
  NodeType f;   // flags, 5 bits left
  std::int16_t s;    // score
  Move m;       // move
  std::int16_t e;    // eval

  friend struct HashTable;
};
#pragma pack()

struct HashTable final
{
private:
  friend struct HashEntry;

  static constexpr std::size_t BucketSize = 4;

  using BucketArray = std::array<HashEntry, BucketSize>;

  // Just use a simple array for bucket
  struct Bucket final
  {
    alignas(CacheLineSize) BucketArray entry{};
  };

public:
  ~HashTable();
  constexpr HashTable()             = default;
  HashTable(const HashTable &other) = delete;
  HashTable(HashTable &&other)      = delete;
  HashTable &operator=(const HashTable &) = delete;
  HashTable &operator=(HashTable &&other) = delete;

  void init(std::uint64_t newSizeMb);

  void clear();

  void initSearch();

  [[nodiscard]]
  HashEntry *firstEntry(const Key key) const
  {
    return &table_[mul_hi64(key, m_bucketCount)].entry[0];
  }

  [[nodiscard]]
  Bucket *findBucket(const Key key) const
  {
    return &table_[mul_hi64(key, m_bucketCount)];
  }

  [[nodiscard]]
  HashEntry *find(Key key) const;

  HashEntry *insert(Key key, int depth, int score, NodeType nt, Move m, int eval);

  void insert(const PVEntry &pv);

  [[nodiscard]]
  HashEntry *getEntryToReplace(Key key, [[maybe_unused]] int depth) const;

  [[nodiscard]]
  int load() const;

  [[nodiscard]]
  int sizeMb() const;

private:
  static_assert(CacheLineSize % sizeof(Bucket) == 0, "Bucket size incorrect");

  Bucket *table_{};
  void *mem_{};

  std::size_t m_bucketCount{};
  std::size_t m_fullnessElement{};
  std::uint64_t m_occupied{};
  std::uint64_t m_sizeMb{};
  std::uint64_t m_size{};
  int m_age{};
};

inline void HashTable::initSearch()
{
  m_age++;
}

inline int HashTable::load() const
{
  return static_cast<int>(static_cast<double>(m_occupied) / m_fullnessElement * 1000);
}

inline int HashTable::sizeMb() const
{
  return static_cast<int>(m_sizeMb);
}

constinit inline HashTable TT;
