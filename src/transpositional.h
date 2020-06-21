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
  uint32_t move;
  int16_t eval;

  [[nodiscard]]
  Move m() const noexcept { return static_cast<Move>(move); }

  [[nodiscard]]
  bool is_exact() const noexcept { return flags & EXACT; }

  [[nodiscard]]
  bool is_beta() const noexcept { return flags & BETA; }

  [[nodiscard]]
  bool is_alpha() const noexcept { return flags & ALPHA; }
};
#pragma pack()

struct HashTable final {
  void init(uint64_t new_size_mb);

  void clear();

  void init_search();

  [[nodiscard]]
  HashEntry *find(Key key) const;

  HashEntry *insert(Key key, int depth, int score, NodeType type, int move, int eval);

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
