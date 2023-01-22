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

#include <cstdint>
#include <iterator>
#include <utility>
#include <iostream>

[[nodiscard]]
auto sum_values(const std::uint8_t *data, const std::size_t size)
{
  constexpr auto scale = 1000;

  auto value = 0;
  for (std::size_t offset = 0; offset < size; ++offset)
    value += static_cast<int>(*std::next(data, static_cast<long>(offset))) * scale;

  return value;
}

// Fuzzer that attempts to invoke undefined behavior for signed integer overflow
// cppcheck-suppress unusedFunction symbolName=LLVMFuzzerTestOneInput
extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t *Data, std::size_t Size)
{
  std::cout << "Value sum: " << sum_values(Data,Size) << ", len:" << Size;
  return 0;
}