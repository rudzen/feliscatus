#pragma once

#include <cstdint>
#include <cmath>
#include "util.h"

#pragma pack(1)
struct HashEntry {
  uint32_t key;
  uint16_t age;// 7 bits left
  uint8_t depth;
  HashNodeType flags;// 5 bits left
  int16_t score;
  uint32_t move;
  int16_t eval;
};
#pragma pack()

class HashTable {
public:
  HashTable() = default;
  explicit HashTable(uint64_t mb) {
    if constexpr (sizeof(HashEntry) != 16)
    {
      printf("error sizeof(HashEntry) == %d\n", static_cast<int>(sizeof(HashEntry)));
      exit(0);
    }
    init(mb);
  }

  void init(uint64_t new_size_mb) {
    new_size_mb = pow2(std::log2(new_size_mb));

    if (new_size_mb == size_mb)
      return;

    size_mb = new_size_mb;
    size    = 1024 * 1024 * size_mb / sizeof(HashEntry);
    mask    = size - 1;
    size += NUMBER_SLOTS - 1;
    delete[] table;
    table = new HashEntry[size];
    clear();
  }

  void clear() {
    memset(table, 0, size * sizeof(HashEntry));
    occupied = 0;
    age      = 0;
  }

  void init_search() { age++; }

  [[nodiscard]]
  HashEntry *find(const uint64_t key) const {
    auto *transp = table + (key & mask);
    const auto k32 = key32(key);
    for (auto i = 0; i < NUMBER_SLOTS; i++, transp++)
    {
      if (transp->key == k32 && transp->flags)
        return transp;
    }
    return nullptr;
  }

  HashEntry *insert(const uint64_t key, const int depth, const int score, const HashNodeType type, const int move, int eval) {
    auto *transp = get_entry_to_replace(key, depth);

    if (transp->flags == 0)
      occupied++;

    const auto k32 = key32(key);

    transp->move  = transp->key != k32 || move != 0 ? move : transp->move;
    transp->key   = k32;
    transp->score = static_cast<int16_t>(score);
    transp->depth = static_cast<uint8_t>(depth);
    transp->flags = type;
    transp->age   = static_cast<uint16_t>(age);
    transp->eval  = static_cast<int16_t>(eval);
    return transp;
  }

  [[nodiscard]]
  HashEntry *get_entry_to_replace(const uint64_t key, [[maybe_unused]] int depth) const {
    auto *transp = table + (key & mask);
    const auto k32 = key32(key);

    if (transp->flags == 0 || transp->key == k32)
      return transp;

    auto *replace = transp++;
    auto replace_score  = (replace->age << 9) + replace->depth;

    for (auto i = 1; i < NUMBER_SLOTS; i++, transp++)
    {
      if (transp->flags == 0 || transp->key == k32)
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

  [[nodiscard]]
  int get_load() const { return static_cast<int>(static_cast<double>(occupied) / size * 1000); }

  [[nodiscard]]
  int get_size_mb() const { return static_cast<int>(size_mb); }

  static constexpr uint32_t key32(const uint64_t key) {
    return key >> 32;
  }

protected:
  HashEntry *table{};
  uint64_t mask{};
  uint64_t occupied{};
  uint64_t size_mb{};
  uint64_t size{};
  int age{};

  static constexpr int NUMBER_SLOTS = 4;
};

#pragma pack(1)

struct PawnHashEntry {
  uint64_t zkey;
  int16_t eval_mg;
  int16_t eval_eg;
  uint8_t passed_pawn_files[2];
  int16_t unused;
};
#pragma pack()

class PawnHashTable {
public:
  explicit PawnHashTable(const uint64_t size_mb) {
    if constexpr (sizeof(PawnHashEntry) != 16)
    {
      printf("error sizeof(PawnHashEntry) == %d\n", static_cast<int>(sizeof(PawnHashEntry)));
      exit(0);
    }
    init(size_mb);
  }

  void init(const uint64_t size_mb) {
    size = 1024 * 1024 * pow2(log2(size_mb)) / sizeof(PawnHashEntry);
    mask = size - 1;
    delete[] table;
    table = new PawnHashEntry[size];
    clear();
  }

  void clear() const { memset(table, 0, size * sizeof(PawnHashEntry)); }

  [[nodiscard]]
  PawnHashEntry *find(const uint64_t key) const {
    auto *pawnp = table + (key & mask);

    if (pawnp->zkey != key || pawnp->zkey == 0)
      return nullptr;
    return pawnp;
  }

  [[nodiscard]]
  PawnHashEntry *insert(const uint64_t key, const int score_mg, const int score_eg, const std::array<int, 2> &passed_pawn_files) const {
    auto *pawnp        = table + (key & mask);
    pawnp->zkey                 = key;
    pawnp->eval_mg              = static_cast<int16_t>(score_mg);
    pawnp->eval_eg              = static_cast<int16_t>(score_eg);
    pawnp->passed_pawn_files[0] = static_cast<uint8_t>(passed_pawn_files[0]);
    pawnp->passed_pawn_files[1] = static_cast<uint8_t>(passed_pawn_files[1]);
    return pawnp;
  }

protected:
  PawnHashEntry *table{};
  uint64_t size{};
  uint64_t mask{};
};

using Hash = HashTable;
using PawnHash = PawnHashTable;

inline HashTable TT;
