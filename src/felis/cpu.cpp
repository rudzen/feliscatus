// Copyright (C) 2008-2016 Gunnar Harms (Bobcat author)
// Copyright (C) 2017      FireFather (Tomcat author)
// Copyright (C) 2020-2025 Rudy Alex Kohn
// See end of file for extended copyright information.

#include <cstring>
#include <limits>
#include <felis/cpu.hpp>
#include <felis/util.hpp>

void cpu::init(Cpu* c) {
  std::memset(c, 0, sizeof(Cpu));

#if defined(WIN32)
  c->self = GetCurrentProcess();

  SYSTEM_INFO sys_info;
  FILETIME ftime;
  FILETIME fsys;
  FILETIME fuser;

  GetSystemInfo(&sys_info);
  c->num_processors = sys_info.dwNumberOfProcessors;

  GetSystemTimeAsFileTime(&ftime);

  std::memcpy(&c->last_cpu, &ftime, sizeof(FILETIME));

  GetProcessTimes(c->self, &ftime, &ftime, &fsys, &fuser);
  std::memcpy(&c->last_sys_cpu, &fsys, sizeof(FILETIME));
  std::memcpy(&c->last_user_cpu, &fuser, sizeof(FILETIME));


#else
  struct tms time_sample;
  char line[128];

  c->last_cpu      = times(&time_sample);
  c->last_sys_cpu  = time_sample.tms_stime;
  c->last_user_cpu = time_sample.tms_utime;

  FILE* file = fopen("/proc/cpuinfo", "r");
  while (fgets(line, 128, file) != nullptr) {
    if (std::strncmp(line, "processor", 9) == 0)
      c->num_processors++;
  }
  fclose(file);
#endif
}
r64 cpu::usage(Cpu* c) {
  r64 percent;

#if defined(WIN32)
  FILETIME ftime{};
  FILETIME fsys{};
  FILETIME fuser{};

  ULARGE_INTEGER now{};
  ULARGE_INTEGER sys{};
  ULARGE_INTEGER user{};

  GetSystemTimeAsFileTime(&ftime);
  std::memcpy(&now, &ftime, sizeof(FILETIME));

  GetProcessTimes(c->self, &ftime, &ftime, &fsys, &fuser);
  std::memcpy(&sys, &fsys, sizeof(FILETIME));
  std::memcpy(&user, &fuser, sizeof(FILETIME));
  percent = static_cast<double>((sys.QuadPart - c->last_sys_cpu.QuadPart) + (user.QuadPart - c->last_user_cpu.QuadPart));
  percent /= static_cast<double>(now.QuadPart - c->last_cpu.QuadPart);
  percent /= c->num_processors;
  c->last_cpu      = now;
  c->last_user_cpu = user;
  c->last_sys_cpu  = sys;

#else

  struct tms time_sample{};
  const clock_t now = times(&time_sample);

  if (now <= c->last_cpu || time_sample.tms_stime < c->last_sys_cpu || time_sample.tms_utime < c->last_user_cpu) {
    // Overflow detection. Just skip this value.
    percent = std::numeric_limits<double>::min();
  } else {
    percent = (time_sample.tms_stime - c->last_sys_cpu) + (time_sample.tms_utime - c->last_user_cpu);
    percent /= (now - c->last_cpu);
    percent /= c->num_processors;
  }
  c->last_cpu      = now;
  c->last_sys_cpu  = time_sample.tms_stime;
  c->last_user_cpu = time_sample.tms_utime;

#endif

  return percent <= 0. ? 0. : percent * 1000;

}

// Feliscatus, a UCI chess playing engine derived from Tomcat 1.0 (Bobcat 8.0)
// Copyright (C) 2008-2016 Gunnar Harms (Bobcat author)
// Copyright (C) 2017      FireFather (Tomcat author)
// Copyright (C) 2020-2025 Rudy Alex Kohn
//
// Feliscatus is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Feliscatus is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Feliscatus.  If not, see <http://www.gnu.org/licenses/>.