#pragma once

#if defined(NO_PREFETCH)

void prefetch(void *) {}

#else

void prefetch(void* addr) {
  __builtin_prefetch(addr);
}
#endif
