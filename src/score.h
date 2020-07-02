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

#pragma once

#include <cstdint>
#include <cassert>

#include <fmt/format.h>

///
/// Multi-valued type which is used to hold the mg and mg scores.
///

struct Score final {

  [[nodiscard]]
  constexpr Score() = default;
  [[nodiscard]]
  constexpr Score(const int mg, const int eg) : value(static_cast<int>(static_cast<unsigned int>(eg) << 16) + mg) {}
  [[nodiscard]]
  constexpr Score(const int v) : value(v) {}

  [[nodiscard]]
  constexpr Score &operator=(const int v) noexcept {
    value = v;
    return *this;
  }

  [[nodiscard]]
  constexpr bool operator==(const Score &b) const { return value == b.value; }

  constexpr Score &operator+=(const int d2) noexcept {
    this->value += d2;
    return *this;
  }

  constexpr Score &operator-=(const int d2) noexcept {
    value -= d2;
    return *this;
  }


  [[nodiscard]]
  constexpr int eg() const noexcept {
    const union {
      uint16_t u;
      int16_t s;
    } eg = {static_cast<uint16_t>((static_cast<unsigned>(value + 0x8000) >> 16))};
    return static_cast<int>(eg.s);
  }

  [[nodiscard]]
  constexpr int mg() const noexcept {
    const union {
      uint16_t u;
      int16_t s;
    } mg = {static_cast<uint16_t>((static_cast<unsigned>(value)))};
    return static_cast<int>(mg.s);
  }

  [[nodiscard]]
  constexpr int raw() const noexcept { return value; }

  [[nodiscard]]
  constexpr int combine() const noexcept { return mg() - eg(); }

private:
  int value{};
};

constexpr Score ZeroScore = Score(0);

// TODO : Move some operators inside Score

constexpr Score operator+(const Score d1, const int d2) noexcept {
  return d1.raw() + d2;
}

constexpr Score operator-(const Score d1, const int d2) noexcept {
  return d1.raw() - d2;
}

constexpr Score operator-(const Score d1, const Score d2) noexcept {
  return Score(d1.mg() - d2.mg(), d1.eg() - d2.eg());
}

constexpr Score operator-(const Score d) noexcept {
  return -d.raw();
}

constexpr Score &operator+=(Score &d1, const Score d2) noexcept {
  return d1 = Score(d1.mg() + d2.mg(), d1.eg() + d2.eg());
}

constexpr Score &operator-=(Score &d1, Score d2) noexcept {
  return d1 = Score(d1.mg() - d2.mg(), d1.eg() - d2.eg());
}

/// Division of a Score must be handled separately for each term
constexpr Score operator/(const Score s, const int i) {
  return Score(s.mg() / i, s.eg() / i);
}

Score operator*(Score, Score) = delete;

/// Multiplication of a Score by an integer. We check for overflow in debug mode.
constexpr Score operator*(const Score s, const int i) {

  const auto result = Score(s.raw() * i);

  assert(result.eg() == i * s.eg());
  assert(result.mg() == i * s.mg());
  assert(i == 0 || result / i == s);

  return result;
}

/// Multiplication of a Score by a boolean
constexpr Score operator*(const Score s, const bool b) {
  return b ? s : ZeroScore;
}

///
/// Score formatter for fmt
///
template<>
struct fmt::formatter<Score> : formatter<std::string_view> {
  // parse is inherited from formatter<string_view>.
  template<typename FormatContext>
  auto format(const Score s, FormatContext &ctx) {
    return formatter<std::string_view>::format(fmt::format("m:{} e:{}", s.mg(), s.eg()), ctx);
  }
};
