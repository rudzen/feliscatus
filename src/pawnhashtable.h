#pragma once

#include <cstdint>
#include <optional>

#include "types.h"
#include "hash.h"

struct Position;

#pragma pack(1)
struct PawnHashEntry final {
  Key zkey;
  int16_t eval_mg;
  int16_t eval_eg;
  std::array<uint8_t, COL_NB> passed_pawn_files;
  int16_t unused;
};
#pragma pack()

struct PawnHashTable final : Table<PawnHashEntry, 512 * sizeof(PawnHashEntry)> {
  [[nodiscard]]
  std::optional<PawnHashEntry *> find(const Position *pos);

  [[nodiscard]]
  PawnHashEntry *insert(Key key, int score_mg, int score_eg, const std::array<int, 2> &passed_pawn_files);
};
