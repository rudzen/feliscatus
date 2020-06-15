#pragma once

#include <cstring>
#include <cmath>

namespace util {

inline double sigmoid(const double x, const double k) {
  return 1 / (1 + std::pow(10, -k * x / 400));
}

constexpr bool iswhitespace(const char c) {
  return c == ' ' || c == '\t' || static_cast<int>(c) == 10 || static_cast<int>(c) == 13;
}

inline char *rtrim(char *buf) {
  while (std::strlen(buf) && iswhitespace(buf[std::strlen(buf) - 1]))
    buf[std::strlen(buf) - 1] = 0;
  return buf;
}

inline char *ltrim(char *buf) {
  while (iswhitespace(buf[0]))
    std::memcpy(buf, buf + 1, strlen(buf));
  return buf;
}

inline char *trim(char *buf) {
  return ltrim(rtrim(buf));
}

inline int tokenize(char *input, char *tokens[], const int max_tokens) {
  auto num_tokens = 0;
  auto *token     = strtok(input, " ");

  while (token != nullptr && num_tokens < max_tokens)
  {
    tokens[num_tokens] = new char[std::strlen(token) + 1];
    strcpy(tokens[num_tokens++], token);
    token = strtok(nullptr, " ");
  }
  return num_tokens;
}

constexpr static int pow2(const int x) {
  return x * x;
}

inline bool strieq(const char *s1, const char *s2) {
  if (std::strlen(s1) != std::strlen(s2))
    return false;

  for (std::size_t i = 0; i < std::strlen(s1); i++)
    if (::tolower(*(s1 + i)) != ::tolower(*(s2 + i)))
      return false;

  return true;
}

inline const char *FENfromParams(const char *params[], const int num_params, int &param, char *fen) {
  if (num_params - param - 1 < 6)
    return nullptr;

  fen[0] = 0;

  for (auto i = 0; i < 6; i++)
  {
    if (std::strlen(fen) + std::strlen(params[++param]) + 1 >= 128)
      return nullptr;

    sprintf(&fen[strlen(fen)], "%s ", params[param]);
  }
  return fen;
}

template<typename T>
constexpr bool in_between(const T value, const T min, const T max) {
  static_assert(std::is_integral<T>::value || std::is_enum<T>::value, "invalid type.");
  return (static_cast<unsigned int>(value) - static_cast<unsigned int>(min) <= static_cast<unsigned int>(max) - static_cast<unsigned int>(min));
}

template<int Min, int Max>
constexpr bool in_between(const int value) {
  return (static_cast<unsigned int>(value) - static_cast<unsigned int>(Min) <= static_cast<unsigned int>(Max) - static_cast<unsigned int>(Min));
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

  return str.front() == '-' ? str.remove_prefix(1), -sv_val() : sv_val();
}

template<typename ToCheck, std::size_t ExpectedSize, std::size_t RealSize = sizeof(ToCheck)>
void check_size() {
  static_assert(ExpectedSize == RealSize, "Size is off!");
}

}
