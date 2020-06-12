#pragma once

#include <windows.h>
#include <sys/timeb.h>
#include <ctime>
#include <cstdint>
#include <cstring>
#include <cmath>

constexpr double sigmoid(const double x, const double k) {
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
    memcpy(buf, buf + 1, strlen(buf));
  return buf;
}

inline char *trim(char *buf) {
  return ltrim(rtrim(buf));
}

inline int tokenize(char *input, char *tokens[], const int max_tokens) {
  int num_tokens = 0;
  char *token    = strtok(input, " ");

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
  if ((num_params - param - 1) < 6)
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

//#if defined(_MSC_VER)

const char *dateAndTimeString(char *buf) {
  auto now      = time(nullptr);
  auto *const time = localtime(&now);
  sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d", time->tm_year + 1900, time->tm_mon + 1, time->tm_mday, time->tm_hour, time->tm_min, time->tm_sec);
  return buf;
}

const char *timeString(char *buf) {
  struct _timeb tb;
  _ftime(&tb);
  auto *const time = localtime(&tb.time);
  sprintf(buf, "%02d:%02d:%02d.%03d", time->tm_hour, time->tm_min, time->tm_sec, tb.millitm);
  return buf;
}

class Stopwatch {
public:
  LARGE_INTEGER start1;
  static LARGE_INTEGER frequency;
  uint64_t start2;

  Stopwatch() { start(); }

  explicit Stopwatch(int) { QueryPerformanceFrequency(&frequency); }

  void start() {
    QueryPerformanceCounter(&start1);
    start2 = GetTickCount64();
  }

  [[nodiscard]]
  uint64_t micros_elapsed_high_res() const {
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return (now.QuadPart - start1.QuadPart) * 1000000 / frequency.QuadPart;
  }

  [[nodiscard]]
  uint64_t millis_elapsed_high_res() const {
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return (now.QuadPart - start1.QuadPart) * 1000 / frequency.QuadPart;
  }

  [[nodiscard]]
  uint64_t millisElapsed() const { return GetTickCount64() - start2; }
};

LARGE_INTEGER Stopwatch::frequency;
inline Stopwatch stopwatch_init(1);