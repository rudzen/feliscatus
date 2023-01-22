/*
  Feliscatus, a UCI chess playing engine derived from Tomcat 1.0 (Bobcat 8.0)
  Copyright (C) 2008-2016 Gunnar Harms (Bobcat author)
  Copyright (C) 2017      FireFather (Tomcat author)
  Copyright (C) 2020-2022 Rudy Alex Kohn

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

#include <cassert>
#if defined(_WIN32)
#if _WIN32_WINNT < 0x0601
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0601   // Force to include needed API prototypes
#endif

#if !defined(NOMINMAX)
#define NOMINMAX
#endif

#include <windows.h>
// The needed Windows API for processor groups could be missed from old Windows
// versions, so instead of calling them directly (forcing the linker to resolve
// the calls at compile time), try to load them at runtime. To do this we need
// first to define the corresponding function pointers.
extern "C" {
typedef bool (*fun1_t)(LOGICAL_PROCESSOR_RELATIONSHIP, PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX, PDWORD);
typedef bool (*fun2_t)(USHORT, PGROUP_AFFINITY);
typedef bool (*fun3_t)(HANDLE, CONST GROUP_AFFINITY *, PGROUP_AFFINITY);
}
#endif

#include <vector>
#include <optional>
#include <sstream>
#include <cstdint>

#include <fmt/format.h>

#include "miscellaneous.hpp"
#include "util.hpp"

namespace
{

std::string compiler_info()
{
#if defined(__clang__)
  return fmt::format("[Clang/LLVM {}{}{}]", std::string(__clang_major__), __clang_minor__, __clang_patchlevel__);
#elif defined(__GNUC__) || defined(__GNUG__)
  return fmt::format("[GNU GCC {}]", std::string(__VERSION__));
#elif defined(_MSC_VER)
  return fmt::format("[MS Visual Studio {}]", _MSC_VER);
#else
  return std::string("Unknown compiler");
#endif
}

}   // namespace

namespace WinProcGroup
{

#ifndef _WIN32

void bind_this_thread(std::size_t)
{ }

#else

/// best_group() retrieves logical processor information using Windows specific
/// API and returns the best group id for the thread with index idx. Original
/// code from Texel by Peter Österlund.

std::optional<int> best_group(const std::size_t idx)
{
  auto threads = 0;
  auto nodes = 0;
  auto cores = 0;
  DWORD return_length = 0;
  DWORD byte_offset = 0;

  // Early exit if the needed API is not available at runtime
  auto *const k32 = GetModuleHandle("Kernel32.dll");
  const auto fun1 =
    reinterpret_cast<fun1_t>(reinterpret_cast<void (*)()>(GetProcAddress(k32, "GetLogicalProcessorInformationEx")));
  if (!fun1)
    return std::nullopt;

  // First call to get returnLength. We expect it to fail due to null buffer
  if (fun1(RelationAll, nullptr, &return_length))
    return std::nullopt;

  // Once we know returnLength, allocate the buffer
  SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *buffer;
  auto *ptr = buffer = static_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *>(malloc(return_length));

  // Second call, now we expect to succeed
  if (!fun1(RelationAll, buffer, &return_length))
  {
    std::free(buffer);
    return std::nullopt;
  }

  while (byte_offset < return_length)
  {
    if (ptr->Relationship == RelationNumaNode)
      nodes++;

    else if (ptr->Relationship == RelationProcessorCore)
    {
      cores++;
      threads += ptr->Processor.Flags == LTP_PC_SMT ? 2 : 1;
    }

    assert(ptr->Size);
    byte_offset += ptr->Size;
    ptr = reinterpret_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *>(reinterpret_cast<char *>(ptr) + ptr->Size);
  }

  std::free(buffer);

  std::vector<int> groups;

  // Run as many threads as possible on the same node until core limit is
  // reached, then move on filling the next node.
  for (auto n = 0; n < nodes; n++)
    for (auto i = 0; i < cores / nodes; i++)
      groups.emplace_back(n);

  // In case a core has more than one logical processor (we assume 2) and we
  // have still threads to allocate, then spread them evenly across available
  // nodes.
  for (auto t = 0; t < threads - cores; t++)
    groups.emplace_back(t % nodes);

  // If we still have more threads than the total number of logical processors
  // then return -1 and let the OS to decide what to do.
  return idx < groups.size() ? std::make_optional(groups[idx]) : std::nullopt;
}


/// bind_this_thread() set the group affinity of the current thread

void bind_this_thread(const std::size_t idx)
{
  // Use only local variables to be thread-safe
  const auto group = best_group(idx);

  [[unlikely]]
  if (!group)
    return;

  // Early exit if the needed API are not available at runtime
  auto *const k32 = GetModuleHandle("Kernel32.dll");
  const auto fun2 =
    reinterpret_cast<fun2_t>(reinterpret_cast<void (*)()>(GetProcAddress(k32, "GetNumaNodeProcessorMaskEx")));
  const auto fun3 =
    reinterpret_cast<fun3_t>(reinterpret_cast<void (*)()>(GetProcAddress(k32, "SetThreadGroupAffinity")));

  if (!fun2 || !fun3)
    return;

  GROUP_AFFINITY affinity;
  if (fun2(group.value(), &affinity))
    fun3(GetCurrentThread(), &affinity, nullptr);
}

#endif

}   // namespace WinProcGroup

namespace misc
{

template<bool AsUci>
std::string print_engine_info()
{
  static constexpr std::string_view title_short{"FelisCatus"};
  static constexpr std::string_view all_months{"Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec"};
  std::string m, d, y, uci;
  fmt::memory_buffer ver_info;
  auto inserter = std::back_inserter(ver_info);

  std::stringstream date(std::string(__DATE__));

  date >> m >> d >> y;

  const auto month    = (1 + all_months.find(m) / 4);
  const auto day      = util::to_integral<std::uint32_t>(d);
  const auto year     = y.substr(y.size() - 2);
  const auto compiler = compiler_info();

  if constexpr (AsUci)
  {
    constexpr std::string_view authors{"Gunnar Harms, FireFather, Rudy Alex Kohn"};
    fmt::format_to(inserter, "id name {} {:02}-{:02}-{} {}\nid author {}", title_short, month, day, year, compiler, authors);
  } else
    fmt::format_to(inserter, "{} {:02}-{:02}-{} {}", title_short, month, day, year, compiler);

#if defined(NO_LAZY_EVAL_THRESHOLD)

  fmt::format_to(inserter, " - NO LAZYEVAL");

#endif

#if defined(NO_PREFETCH)

  fmt::format_to(inserter, " - NO PREFETCH")

#endif

    return fmt::format("{}\n", fmt::to_string(ver_info));
}

template std::string misc::print_engine_info<true>();
template std::string misc::print_engine_info<false>();

}   // namespace misc