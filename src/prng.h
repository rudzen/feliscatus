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

#include <cassert>
#include <concepts>
#include "types.h"

template <class T>
concept PRNGCompatible = std::is_convertible_v<T, Key>;

/// xorshift64star Pseudo-Random Number Generator
/// This class is based on original code written and dedicated
/// to the public domain by Sebastiano Vigna (2014).
/// It has the following characteristics:
///
///  -  Outputs 64-bit numbers
///  -  Passes Dieharder and SmallCrush test batteries
///  -  Does not require warm-up, no zeroland to escape
///  -  Internal state is a single 64-bit integer
///  -  Period is 2^64 - 1
///  -  Speed: 1.60 ns/call (Core i7 @3.40GHz)
///
/// For further analysis see
///   <http://vigna.di.unimi.it/ftp/papers/xorshift.pdf>

template<typename T> requires PRNGCompatible<T>
struct PRNG final {

  PRNG() = delete;
  constexpr explicit PRNG(const T seed) : s(seed) {
    assert(seed);
  }
  ~PRNG()                       = default;
  PRNG(const PRNG &other)       = delete;
  PRNG(PRNG &&other)            = delete;
  PRNG &operator=(const PRNG &) = delete;
  PRNG &operator=(PRNG &&other) = delete;

  [[nodiscard]]
  constexpr T operator()() noexcept { return T(rand64()); }

  template<typename T2> requires PRNGCompatible<T>
  [[nodiscard]]
  constexpr T rand() noexcept { return T2(rand64()); }

  /// Special generator used to fast init magic numbers.
  /// Output values only have 1/8th of their bits set on average.
  template<typename T2> requires PRNGCompatible<T>
  [[nodiscard]]
  constexpr T sparse_rand() { return T2(rand64() & rand64() & rand64()); }

 private:
  uint64_t s;

  [[nodiscard]]
  constexpr uint64_t rand64() {

    s ^= s >> 12, s ^= s << 25, s ^= s >> 27;
    return s * 2685821657736338717LL;
  }
};
