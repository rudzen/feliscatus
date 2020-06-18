#include <fmt/format.h>
#include "pawnhashtable.h"
#include "position.h"

PawnHashEntry *PawnHashTable::find(const Position *pos) {
  return (*this)[pos->pawn_structure_key];
}

PawnHashEntry *PawnHashTable::insert(const Key key, const int score_mg, const int score_eg, const std::array<int, 2> &passed_pawn_files) {
  auto *pawnp    = (*this)[key];
  pawnp->zkey    = key;
  pawnp->eval_mg = static_cast<int16_t>(score_mg);
  pawnp->eval_eg = static_cast<int16_t>(score_eg);
  std::copy(passed_pawn_files.begin(), passed_pawn_files.end(), pawnp->passed_pawn_files.begin());
  return pawnp;
}
