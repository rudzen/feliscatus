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

#include <thread>
#include <cstdint>
#include <cstdlib>
#include <vector>

#include <fmt/format.h>

#include "transpositional.hpp"
#include "uci.hpp"

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

void HashTable::init(const std::uint64_t new_size_mb)
{
  if (size_mb_ == new_size_mb)
    return;

  // Original code from SF

  bucket_count_     = new_size_mb * 1024 * 1024 / sizeof(Bucket);
  fullness_element_ = bucket_count_ * BucketSize;
  std::free(mem_);
  size_ = bucket_count_ * sizeof(Bucket) + CacheLineSize - 1;
  // TODO : replace with std::aligned_alloc() at some point;
  mem_ = std::malloc(size_);

  if (!mem_)
  {
    fmt::print(stderr, "Failed to allocate {}MB for transposition table.\n", new_size_mb);
    exit(EXIT_FAILURE);
  }

  table_   = reinterpret_cast<Bucket *>((uintptr_t(mem_) + CacheLineSize - 1) & ~(CacheLineSize - 1));
  size_mb_ = new_size_mb;
  clear();
}

void HashTable::clear()
{
  // Original code from SF

  const auto thread_count = static_cast<std::size_t>(Options.at(uci::uci_name<uci::UciOptions::THREADS>()));
  std::vector<std::jthread> threads(thread_count);

  for (std::size_t idx = 0; idx < thread_count; idx++)
  {
    threads.emplace_back(std::jthread([this, idx, thread_count]() {
      // Thread binding gives faster search on systems with a first-touch policy
      if (thread_count > 8)
        WinProcGroup::bind_this_thread(idx);

      // Each thread will zero its part of the hash table
      const auto stride = bucket_count_ / thread_count, start = stride * idx,
                 len = idx != thread_count - 1 ? stride : bucket_count_ - start;

      // treat as void* to shut up compiler warning -Wclass-memaccess as this is "totally" safe
      std::memset(reinterpret_cast<void *>(&table_[start]), 0, len * sizeof(Bucket));
    }));
  }
}

HashEntry *HashTable::find(const Key key) const
{
  auto *bucket     = find_bucket(key);
  const auto k32   = key32(key);
  const auto found = std::find_if(bucket->entry.begin(), bucket->entry.end(), [&k32](const HashEntry &e) {
    return e.k == k32 && e.f;
  });
  return found != bucket->entry.end() ? found : nullptr;
}

HashEntry *
  HashTable::insert(const Key key, const int depth, const int score, const NodeType nt, const Move m, const int eval)
{
  auto *transp = get_entry_to_replace(key, depth);

  if (transp->f == NO_NT)
    occupied_++;

  const auto k32 = key32(key);

  if (transp->k != k32 || m != MOVE_NONE)
    transp->m = m;

  transp->k = k32;
  transp->s = static_cast<std::int16_t>(score);
  transp->d = static_cast<std::uint8_t>(depth);
  transp->f = nt;
  transp->a = static_cast<std::uint16_t>(age_);
  transp->e = static_cast<std::int16_t>(eval);
  return transp;
}

HashEntry *HashTable::get_entry_to_replace(const Key key, [[maybe_unused]] const int depth) const
{
  auto *bucket   = find_bucket(key);
  const auto k32 = key32(key);

  auto *entry = &bucket->entry.front();

  if (entry->f == NO_NT || entry->k == k32)
    return entry;

  constexpr auto replacement_score = [](const HashEntry *e) {
    return (e->a << 9) + e->d;
  };
  auto match = [&k32](HashEntry *e) {
    return e->f == NO_NT || e->k == k32;
  };
  auto *replace      = entry;
  auto replace_score = replacement_score(replace);

  // Returns true if match is found, otherwise it updates the potential replacer entry
  const auto replacer = [&](HashEntry &e) {
    if (match(&e))
      return true;

    if (const auto score = replacement_score(&e); score < replace_score)
    {
      replace_score = score;
      replace       = &e;
    }

    return false;
  };

  // Attempt to find a suitable replacement,
  // skips the first entry as it has been checked at the start of the function
  auto found = std::find_if(std::next(bucket->entry.begin()), bucket->entry.end(), replacer);

  return found != bucket->entry.end() ? found : replace;
}
