#pragma once

#include <cstdint>
#include <array>
#include "types.h"

template<typename Entry, std::size_t N>
struct Table {
  [[nodiscard]]
  Entry *operator[](const Key key) noexcept { return &table_[static_cast<uint32_t>(key) & (N - 1)]; }

private:
  std::array<Entry, N> table_{};
};
