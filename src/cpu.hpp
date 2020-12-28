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

#if defined(_WIN32) || defined(_MSC_VER)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#else
#include <string>
#include <cstdlib>
#include <cstdio>
#include <array>
#include <cstring>
#include "sys/times.h"
#include "sys/vtimes.h"
#endif

struct CpuLoad final
{

  CpuLoad();
  ~CpuLoad()                    = default;
  CpuLoad(const CpuLoad &other) = delete;
  CpuLoad(CpuLoad &&other)      = delete;
  CpuLoad &operator=(const CpuLoad &) = delete;
  CpuLoad &operator=(CpuLoad &&other) = delete;

  [[nodiscard]]
  int usage();

private:
#if defined(WIN32)

  ULARGE_INTEGER last_cpu{};
  ULARGE_INTEGER last_sys_cpu{};
  ULARGE_INTEGER last_user_cpu{};
  DWORD num_processors{};
  HANDLE self{};

#else

  clock_t last_cpu{};
  clock_t last_sys_cpu{};
  clock_t last_user_cpu{};
  int num_processors{};

#endif
};
