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

#include <concepts>
#include <cmath>
#include <string>
#include <string_view>

#if defined(WIN32) || defined(_MSC_VER)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#else
#include <thread>
#include <climits>
#include <emmintrin.h>
#endif

namespace util {

template<typename T>
constexpr T abs(const T v) {
  static_assert(std::is_integral_v<T>);
  constexpr auto mask_shift = (sizeof(int) * CHAR_BIT - 1);
  const auto mask = static_cast<int>(v) >> mask_shift;
  return (v ^ mask) - mask;
}

constexpr void sleep(const std::integral auto ms) {
#if defined(__linux__)
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
#else
  Sleep(static_cast<DWORD>(ms));
#endif
}

constexpr double sigmoid(const double x, const double k) {
  return 1 / (1 + std::pow(10, -k * x / 400));
}

template<typename T>
constexpr bool in_between(const T v, const T min, const T max) {
  static_assert(std::is_integral<T>::value || std::is_enum<T>::value, "invalid type.");
  return static_cast<unsigned int>(v) - static_cast<unsigned int>(min) <= static_cast<unsigned int>(max) - static_cast<unsigned int>(min);
}

template<int Min, int Max>
constexpr bool in_between(const int v) {
  return static_cast<unsigned int>(v) - static_cast<unsigned int>(Min) <= static_cast<unsigned int>(Max) - static_cast<unsigned int>(Min);
}

template<typename Integral>
constexpr char to_char(const Integral v) {
  return static_cast<char>(v + '0');
}

template<typename T>
constexpr T from_char(const char c) {
  return static_cast<T>(c - '0');
}

template<typename T>
constexpr T to_integral(std::string_view str) {

  static_assert(std::is_integral_v<T>, "Only integrals allowed.");

  auto sv_val = [&str]() {
    auto x = T(0);
    while (in_between<'0', '9'>(str.front()))
    {
      x = x * 10 + from_char<T>(str.front());
      str.remove_prefix(1);
    }
    return x;
  };

  if constexpr (std::is_signed_v<T>)
  {
    return str.front() == '-'
         ? str.remove_prefix(1), -sv_val()
         : sv_val();
  } else
  {
    if (str.front() == '-')
      str.remove_prefix(1);
    return sv_val();
  }

}

template<typename ToCheck, std::size_t ExpectedSize, std::size_t RealSize = sizeof(ToCheck)>
void check_size() {
  static_assert(ExpectedSize == RealSize, "Size is off!");
}

/**
 * Round to the nearest integer (based on idea from OpenCV)
 * @param value The value to round
 * @return Nearest integer
 */
template<typename T, typename T2>
constexpr T round(T2 value) {
  static_assert(std::is_integral<T>::value || std::is_trivial<T>::value, "round only returns integral.");
  static_assert(std::is_floating_point<T2>::value, "round is only possible for floating points.");
#if ((defined(_MSC_VER) && defined(_M_X64)) || (defined(__GNUC__) && defined(__x86_64__) && defined(__SSE2__) && !defined(__APPLE__)) || defined(CV_SSE2)) && !defined(__CUDACC__)
  const __m128d t = _mm_set_sd(value);
  return static_cast<T>(_mm_cvtsd_si32(t));
#elif defined(_MSC_VER) && defined(_M_IX86)
  int t;
  __asm
  {
      fld value;
      fistp t;
  }
  return static_cast<T>(t);
#elif ((defined(_MSC_VER) && defined(_M_ARM)) || defined(CV_ICC) || defined(__GNUC__)) && defined(HAVE_TEGRA_OPTIMIZATION)
  TEGRA_ROUND_DBL(value);
#elif defined(CV_ICC) || defined(__GNUC__)
#if defined(ARM_ROUND_DBL)
  ARM_ROUND_DBL(value);
#else
  return static_cast<T>(lrint(value));
#endif
#else
  /* it's ok if round does not comply with IEEE754 standard;
  the tests should allow +/-1 difference when the tested functions use round */
#if defined(CV_VERSION)
  return static_cast<T>(cv::floor(d + 0.5));
#else
  return static_cast<T>(floor(d + 0.5));
#endif

#endif
}

inline void find_and_replace(std::string &source, const std::string_view &find, const std::string_view &replace, bool only_once = true) {
  for (std::string::size_type i = 0; (i = source.find(find, i)) != std::string::npos;)
  {
    source.replace(i, find.length(), replace);
    if (only_once)
      return;
    i += replace.length();
  }
}

}
