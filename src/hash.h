#pragma once

#include <cstdint>
#include <cmath>
#include "util.h"
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
};
#pragma pack()

struct HashTable {
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
    new_size_mb = util::pow2(std::log2(new_size_mb));

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
    std::memset(table, 0, size * sizeof(HashEntry));
    occupied = 0;
    age      = 0;
  }

  void init_search() { age++; }

  [[nodiscard]] HashEntry *find(const Key key) const {
    auto *transp   = table + (key & mask);
    const auto k32 = key32(key);
    for (auto i = 0; i < NUMBER_SLOTS; i++, transp++)
    {
      if (transp->key == k32 && transp->flags)
        return transp;
    }
    return nullptr;
  }

  HashEntry *insert(const Key key, const int depth, const int score, const NodeType type, const int move, int eval) {
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

  [[nodiscard]] HashEntry *get_entry_to_replace(const Key key, [[maybe_unused]] int depth) const {
    auto *transp   = table + (key & mask);
    const auto k32 = key32(key);

    if (transp->flags == 0 || transp->key == k32)
      return transp;

    auto *replace      = transp++;
    auto replace_score = (replace->age << 9) + replace->depth;

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

  [[nodiscard]] int get_load() const { return static_cast<int>(static_cast<double>(occupied) / size * 1000); }

  [[nodiscard]] int get_size_mb() const { return static_cast<int>(size_mb); }

  static constexpr uint32_t key32(const Key key) { return key >> 32; }

protected:
  HashEntry *table{};
  Bitboard mask{};
  Bitboard occupied{};
  uint64_t size_mb{};
  uint64_t size{};
  int age{};

  static constexpr int NUMBER_SLOTS = 4;
};

#pragma pack(1)

struct PawnHashEntry {
  Key zkey;
  int16_t eval_mg;
  int16_t eval_eg;
  std::array<uint8_t, COL_NB> passed_pawn_files;
  int16_t unused;
};
#pragma pack()

struct PawnHashTable {
  explicit PawnHashTable(const uint64_t size_mb) {
    if constexpr (sizeof(PawnHashEntry) != 16)
    {
      printf("error sizeof(PawnHashEntry) == %d\n", static_cast<int>(sizeof(PawnHashEntry)));
      exit(0);
    }
    init(size_mb);
  }

  void init(const uint64_t size_mb) {
    size = 1024 * 1024 * util::pow2(log2(size_mb)) / sizeof(PawnHashEntry);
    mask = size - 1;
    delete[] table;
    table = new PawnHashEntry[size];
    clear();
  }

  void clear() const { std::memset(table, 0, size * sizeof(PawnHashEntry)); }

  [[nodiscard]]
  PawnHashEntry *find(const uint64_t key) const {
    auto *pawnp = table + (key & mask);

    if (pawnp->zkey != key || pawnp->zkey == 0)
      return nullptr;
    return pawnp;
  }

  [[nodiscard]]
  PawnHashEntry *insert(const uint64_t key, const int score_mg, const int score_eg, const std::array<int, 2> &passed_pawn_files) const {
    auto *pawnp    = table + (key & mask);
    pawnp->zkey    = key;
    pawnp->eval_mg = static_cast<int16_t>(score_mg);
    pawnp->eval_eg = static_cast<int16_t>(score_eg);
    std::copy(passed_pawn_files.begin(), passed_pawn_files.end(), pawnp->passed_pawn_files.begin());
    return pawnp;
  }

protected:
  PawnHashEntry *table{};
  uint64_t size{};
  uint64_t mask{};
};

using Hash     = HashTable;
using PawnHash = PawnHashTable;

inline HashTable TT;
