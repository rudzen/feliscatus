#pragma once

#include <chrono>

using TimeUnit = std::chrono::milliseconds::rep;

#if defined(NO_PREFETCH)

inline void prefetch(void *) {}

#else

inline void prefetch(void* addr) {
  __builtin_prefetch(addr);
}
#endif
