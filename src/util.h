#pragma once

#include <concepts>
#include <cmath>

#if defined(WIN32) || defined(_MSC_VER)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#else
#include <thread>
#endif

namespace util {

template<typename T>
constexpr int abs(const T v)
{
  static_assert(std::is_integral_v<T> && std::is_signed_v<T>);
  return v < 0 ? -v : v;
}

constexpr void sleep(const std::integral auto ms) {
#ifdef __linux__
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
#else
  Sleep(static_cast<DWORD>(ms));
#endif
}

inline double sigmoid(const double x, const double k) {
  return 1 / (1 + std::pow(10, -k * x / 400));
}

template<typename T>
constexpr bool in_between(const T value, const T min, const T max) {
  static_assert(std::is_integral<T>::value || std::is_enum<T>::value, "invalid type.");
  return static_cast<unsigned int>(value) - static_cast<unsigned int>(min) <= static_cast<unsigned int>(max) - static_cast<unsigned int>(min);
}

template<int Min, int Max>
constexpr bool in_between(const int value) {
  return static_cast<unsigned int>(value) - static_cast<unsigned int>(Min) <= static_cast<unsigned int>(Max) - static_cast<unsigned int>(Min);
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

  return str.front() == '-'
       ? str.remove_prefix(1), -sv_val()
       : sv_val();
}

template<typename ToCheck, std::size_t ExpectedSize, std::size_t RealSize = sizeof(ToCheck)>
void check_size() {
  static_assert(ExpectedSize == RealSize, "Size is off!");
}

}
