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

#include <cstring>
#include <limits>

#include "cpu.hpp"
#include "util.hpp"

namespace
{

constexpr double DEFAULT_OVERFLOW_VALUE = std::numeric_limits<double>::min();

}

#if defined(WIN32)

CpuLoad::CpuLoad() : self(GetCurrentProcess())
{
  SYSTEM_INFO sys_info{};
  FILETIME ftime{};
  FILETIME fsys{};
  FILETIME fuser{};

  GetSystemInfo(&sys_info);
  num_processors = sys_info.dwNumberOfProcessors;

  GetSystemTimeAsFileTime(&ftime);

  std::memcpy(&last_cpu, &ftime, sizeof(FILETIME));

  GetProcessTimes(self, &ftime, &ftime, &fsys, &fuser);
  std::memcpy(&last_sys_cpu, &fsys, sizeof(FILETIME));
  std::memcpy(&last_user_cpu, &fuser, sizeof(FILETIME));
}
#else

CpuLoad::CpuLoad()
{
  struct tms time_sample
  {
  };
  std::array<char, 128> line{};

  last_cpu      = times(&time_sample);
  last_sys_cpu  = time_sample.tms_stime;
  last_user_cpu = time_sample.tms_utime;

  FILE *file = fopen("/proc/cpuinfo", "r");
  while (fgets(line.data(), 128, file) != nullptr)
  {
    if (std::strncmp(line.data(), "processor", 9) == 0)
      num_processors++;
  }
  fclose(file);
}

#endif

int CpuLoad::usage()
{
  double percent;

#if defined(WIN32)
  FILETIME ftime{};
  FILETIME fsys{};
  FILETIME fuser{};

  ULARGE_INTEGER now{};
  ULARGE_INTEGER sys{};
  ULARGE_INTEGER user{};

  GetSystemTimeAsFileTime(&ftime);
  std::memcpy(&now, &ftime, sizeof(FILETIME));

  GetProcessTimes(self, &ftime, &ftime, &fsys, &fuser);
  std::memcpy(&sys, &fsys, sizeof(FILETIME));
  std::memcpy(&user, &fuser, sizeof(FILETIME));
  percent = static_cast<double>((sys.QuadPart - last_sys_cpu.QuadPart) + (user.QuadPart - last_user_cpu.QuadPart));
  percent /= static_cast<double>(now.QuadPart - last_cpu.QuadPart);
  percent /= num_processors;
  last_cpu      = now;
  last_user_cpu = user;
  last_sys_cpu  = sys;

#else
  struct tms time_sample
  {
  };
  const clock_t now = times(&time_sample);

  if (now <= last_cpu || time_sample.tms_stime < last_sys_cpu || time_sample.tms_utime < last_user_cpu)
  {
    // Overflow detection. Just skip this value.
    percent = DEFAULT_OVERFLOW_VALUE;
  } else
  {
    percent = (time_sample.tms_stime - last_sys_cpu) + (time_sample.tms_utime - last_user_cpu);
    percent /= (now - last_cpu);
    percent /= num_processors;
  }
  last_cpu      = now;
  last_sys_cpu  = time_sample.tms_stime;
  last_user_cpu = time_sample.tms_utime;

#endif

  return percent <= 0 ? 0 : util::round<int>(percent * 1000);
}
