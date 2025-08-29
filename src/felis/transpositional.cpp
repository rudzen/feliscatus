// Copyright (C) 2008-2016 Gunnar Harms (Bobcat author)
// Copyright (C) 2017      FireFather (Tomcat author)
// Copyright (C) 2020-2025 Rudy Alex Kohn
// See end of file for extended copyright information.

#include <thread>
#include <cstdint>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <fmt/format.h>
#include <felis/transpositional.hpp>
#include <felis/uci.hpp>

namespace
{

constexpr std::uint32_t key32(const Key key)
{
  return key >> 32;
}

}   // namespace

HashTable::~HashTable()
{
  std::free(mem_);
}

void HashTable::init(const std::uint64_t newSizeMb)
{
  if (m_sizeMb == newSizeMb)
    return;

  // Original code from SF

  m_bucketCount      = newSizeMb * 1024 * 1024 / sizeof(Bucket);
  m_fullnessElement  = m_bucketCount * BucketSize;
  std::free(mem_);
  m_size = m_bucketCount * sizeof(Bucket) + CacheLineSize - 1;
  mem_ = std::malloc(m_size);

  if (!mem_)
  {
    fmt::print(stderr, "Failed to allocate {}MB for transposition table.\n", newSizeMb);
    exit(EXIT_FAILURE);
  }

  table_   = reinterpret_cast<Bucket *>((uintptr_t(mem_) + CacheLineSize - 1) & ~(CacheLineSize - 1));
  m_sizeMb = newSizeMb;
  clear();
}

void HashTable::clear()
{
  // Original code from SF

  const auto threadCount = static_cast<std::size_t>(Options[uci::uciName<uci::UciOptions::THREADS>()]);
  std::vector<std::jthread> threads(threadCount);

  for (std::size_t idx = 0; idx < threadCount; idx++)
  {
    threads.emplace_back(std::jthread([this, idx, threadCount]() {
      // Thread binding gives faster search on systems with a first-touch policy
      if (threadCount > 8)
        WinProcGroup::bind_this_thread(idx);

      // Each thread will zero its part of the hash table
      const auto stride = m_bucketCount / threadCount, start = stride * idx,
                 len = idx != threadCount - 1 ? stride : m_bucketCount - start;

      // treat as void* to shut up compiler warning -Wclass-memaccess as this is "totally" safe
      std::memset(reinterpret_cast<void *>(&table_[start]), 0, len * sizeof(Bucket));
    }));
  }
}

HashEntry *HashTable::find(const Key key) const
{
  auto *bucket     = findBucket(key);
  const auto k32   = key32(key);
  const auto found = std::ranges::find_if(bucket->entry.begin(), bucket->entry.end(), [&k32](const HashEntry &e) {
    return e.k == k32 && e.f;
  });
  return found != bucket->entry.end() ? found : nullptr;
}

HashEntry *HashTable::insert(
  const Key key,
  const int depth,
  const int score,
  const NodeType nt,
  const Move m,
  const int eval)
{
  auto *transp = getEntryToReplace(key, depth);

  if (transp->f == NO_NT)
    m_occupied++;

  const auto k32 = key32(key);

  if (transp->k != k32 || m != MOVE_NONE)
    transp->m = m;

  transp->k = k32;
  transp->s = static_cast<std::int16_t>(score);
  transp->d = static_cast<std::uint8_t>(depth);
  transp->f = nt;
  transp->a = static_cast<std::uint16_t>(m_age);
  transp->e = static_cast<std::int16_t>(eval);
  return transp;
}

void HashTable::insert(const PVEntry &pv)
{
  auto *transp = getEntryToReplace(pv.key, pv.depth);

  if (transp->f == NO_NT)
    m_occupied++;

  const auto k32 = key32(pv.key);

  if (transp->k != k32 || pv.move != MOVE_NONE)
    transp->m = pv.move;

  transp->k = k32;
  transp->s = static_cast<std::int16_t>(pv.score);
  transp->d = static_cast<std::uint8_t>(pv.depth);
  transp->f = pv.node_type;
  transp->a = static_cast<std::uint16_t>(m_age);
  transp->e = static_cast<std::int16_t>(pv.eval);
}

HashEntry *HashTable::getEntryToReplace(Key key,
  [[maybe_unused]] int depth) const
{
  auto *bucket   = findBucket(key);
  const auto k32 = key32(key);

  auto *entry = &bucket->entry.front();

  if (entry->f == NO_NT || entry->k == k32)
    return entry;

  constexpr auto replacementScore = [](const HashEntry *e) {
    return (e->a << 9) + e->d;
  };
  auto match = [&k32](const HashEntry *e) {
    return e->f == NO_NT || e->k == k32;
  };
  auto *replace      = entry;
  auto replaceScore  = replacementScore(replace);

  // Returns true if match is found, otherwise it updates the potential replacer entry
  const auto replacer = [&](HashEntry &e) {
    if (match(&e))
      return true;

    if (const auto score = replacementScore(&e); score < replaceScore)
    {
      replaceScore  = score;
      replace       = &e;
    }

    return false;
  };

  // Attempt to find a suitable replacement,
  // skips the first entry as it has been checked at the start of the function
  auto found = std::find_if(std::next(bucket->entry.begin()), bucket->entry.end(), replacer);

  return found != bucket->entry.end() ? found : replace;
}

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