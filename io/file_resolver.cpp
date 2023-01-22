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

#include <fmt/format.h>

#include "file_resolver.hpp"

struct FileResolver final
{

  explicit FileResolver(const std::string_view f) : file_(std::filesystem::path(f))
  { }

  explicit FileResolver(const std::string_view f, const std::string_view post_name)
  {
    fmt::memory_buffer b;
    fmt::format_to(std::back_inserter(b), "{}{}", post_name, f);
    file_ = std::filesystem::path(fmt::to_string(b));
  }

  [[nodiscard]]
  bool exists() const;

  [[nodiscard]]
  bool is_file() const;

  [[nodiscard]]
  std::uintmax_t size() const;

  [[nodiscard]]
  std::filesystem::path file_name() const;

private:
  std::filesystem::path file_;
};

inline bool FileResolver::exists() const
{
  return std::filesystem::exists(file_);
}

inline bool FileResolver::is_file() const
{
  return std::filesystem::is_regular_file(file_);
}

inline std::uintmax_t FileResolver::size() const
{
  if (exists() && is_file())
  {
    const auto filesize = std::filesystem::file_size(file_);
    if (filesize != static_cast<uintmax_t>(-1))
      return filesize;
  }

  return static_cast<uintmax_t>(-1);
}

inline std::filesystem::path FileResolver::file_name() const
{
  return std::filesystem::absolute(file_);
}

template<typename T, typename... Args>
std::unique_ptr<T> file_handler::make_file_resolver(Args &&... args)
{
  static_assert(std::is_same_v<T, FileResolver>);
  return std::make_unique<T>(T(std::forward<Args>(args)...));
}
