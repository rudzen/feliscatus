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

#include <chrono>
#include <string_view>
#include <string>

using TimeUnit = std::chrono::milliseconds::rep;

constexpr std::string_view start_position = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
constexpr std::string_view piece_index{"pnbrqk"};

#if defined(NO_PREFETCH)

inline void prefetch(void *) {}

#else

inline void prefetch(void* addr) {
  __builtin_prefetch(addr);
}
#endif

inline uint64_t mul_hi64(const uint64_t a, const uint64_t b) {
#if defined(__GNUC__)
  __extension__ typedef unsigned __int128 uint128;
  return (static_cast<uint128>(a) * static_cast<uint128>(b)) >> 64;
#else
  const uint64_t aL = static_cast<uint32_t>(a);
  const uint64_t aH = a >> 32;
  const uint64_t bL = static_cast<uint32_t>(b);
  const uint64_t bH = b >> 32;
  const uint64_t c1 = (aL * bL) >> 32;
  const uint64_t c2 = aH * bL + c1;
  const uint64_t c3 = aL * bH + static_cast<uint32_t>(c2);
  return aH * bH + (c2 >> 32) + (c3 >> 32);
#endif
}

/// Under Windows it is not possible for a process to run on more than one
/// logical processor group. This usually means to be limited to use max 64
/// cores. To overcome this, some special platform specific API should be
/// called to set group affinity for each thread. Original code from Texel by
/// Peter Österlund.

namespace WinProcGroup {
  void bind_this_thread(std::size_t idx);
}

namespace misc {
  template<bool AsUci>
  std::string print_engine_info();
}