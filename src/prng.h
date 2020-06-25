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

  [[nodiscard]]
  constexpr T operator()() noexcept { return T(rand64()); }

  template<typename T2>
  [[nodiscard]]
  constexpr T rand() noexcept { return T2(rand64()); }

  /// Special generator used to fast init magic numbers.
  /// Output values only have 1/8th of their bits set on average.
  template<typename T2>
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
