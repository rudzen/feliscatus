#pragma once

#include <windows.h>
#include <sys/timeb.h>
#include <time.h>
#include <minwindef.h>
#include <inttypes.h>
#include <cstring>

constexpr double sigmoid(double x, double K) {
  return 1 / (1 + pow(10, -K * x / 400));
}

constexpr bool iswhitespace(char c) {
  return c == ' ' || c == '\t' || (int)c == 10 || (int)c == 13;
}

inline char *rtrim(char *buf) {
  while (std::strlen(buf) && iswhitespace(buf[std::strlen(buf) - 1]))
  {
    buf[std::strlen(buf) - 1] = 0;
  }
  return buf;
}

inline char *ltrim(char *buf) {
  while (iswhitespace(buf[0]))
  {
    memcpy(buf, buf + 1, strlen(buf));
  }
  return buf;
}

inline char *trim(char *buf) {
  return ltrim(rtrim(buf));
}

inline int tokenize(char *input, char *tokens[], int max_tokens) {
  int num_tokens = 0;
  char *token    = strtok(input, " ");

  while (token != NULL && num_tokens < max_tokens)
  {
    tokens[num_tokens] = new char[std::strlen(token) + 1];
    strcpy(tokens[num_tokens++], token);
    token = strtok(NULL, " ");
  }
  return num_tokens;
}

constexpr static int pow2(int x) {
  return (int)std::pow(2.0, x);
}

inline bool strieq(const char *s1, const char *s2) {
  if (std::strlen(s1) != std::strlen(s2))
  {
    return false;
  }

  for (size_t i = 0; i < std::strlen(s1); i++)
  {
    if (::tolower(*(s1 + i)) != ::tolower(*(s2 + i)))
    {
      return false;
    }
  }
  return true;
}

inline const char *FENfromParams(const char *params[], int num_params, int &param, char *fen) {
  if ((num_params - param - 1) < 6)
  {
    return NULL;
  }
  fen[0] = 0;

  for (int i = 0; i < 6; i++)
  {
    if (std::strlen(fen) + std::strlen(params[++param]) + 1 >= 128)
    {
      return NULL;
    }
    sprintf(&fen[strlen(fen)], "%s ", params[param]);
  }
  return fen;
}

//#if defined(_MSC_VER)

const char *dateAndTimeString(char *buf) {
  time_t now      = time(NULL);
  struct tm *time = localtime(&now);
  sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d", time->tm_year + 1900, time->tm_mon + 1, time->tm_mday, time->tm_hour, time->tm_min, time->tm_sec);
  return buf;
}

const char *timeString(char *buf) {
  struct _timeb tb;
  _ftime(&tb);
  struct tm *time = localtime(&tb.time);
  sprintf(buf, "%02d:%02d:%02d.%03d", time->tm_hour, time->tm_min, time->tm_sec, tb.millitm);
  return buf;
}

class Stopwatch {
public:
  LARGE_INTEGER start1_;
  static LARGE_INTEGER frequency_;
  uint64_t start2_;

  Stopwatch() { start(); }

  Stopwatch(int) { QueryPerformanceFrequency(&frequency_); }

  void start() {
    QueryPerformanceCounter(&start1_);
    start2_ = GetTickCount64();
  }

  uint64_t microsElapsedHighRes() const {
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return (now.QuadPart - start1_.QuadPart) * 1000000 / frequency_.QuadPart;
  }

  uint64_t millisElapsedHighRes() const {
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return (now.QuadPart - start1_.QuadPart) * 1000 / frequency_.QuadPart;
  }

  uint64_t millisElapsed() const { return GetTickCount64() - start2_; }
};

LARGE_INTEGER Stopwatch::frequency_;
static Stopwatch stopwatch_init(1);