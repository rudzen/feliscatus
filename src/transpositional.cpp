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

HashEntry *HashTable::insert(const Key key, const int depth, const int score, const NodeType type, const int move, const int eval) {
  auto *transp = get_entry_to_replace(key, depth);

  if (transp->flags == 0)
    occupied++;

  const auto k32 = key32(key);

  if (transp->key != k32 || move != 0)
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

  auto *replace      = transp++;
  auto replace_score = (replace->age << 9) + replace->depth; //+ depth;

  for (auto i = 1; i < NUMBER_SLOTS; i++, transp++)
  {
    if (transp->flags == Void || transp->key == k32)
      return transp;

    const auto score = (transp->age << 9) + transp->depth;

    if (score < replace_score)
    {
      replace_score = score;
      replace       = transp;
    }
  }
  return replace;
}
