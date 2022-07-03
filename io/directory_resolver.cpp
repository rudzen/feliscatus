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

#include <memory>
#include <filesystem>
#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include "directory_resolver.hpp"

namespace
{

constexpr auto max_log_file_size = 1048576 * 5;
constexpr auto max_log_files     = 3;

const std::shared_ptr<spdlog::logger> logger =
  spdlog::rotating_logger_mt("directory_logger", "logs/directory.log", max_log_file_size, max_log_files);

constexpr std::string_view extension = ".bin";

}

std::vector<std::string> directory_resolver::get_book_list()
{
  namespace fs = std::filesystem;

  auto cwd = fs::current_path().append("polybooks");

  if (!fs::is_directory(cwd)) {
    logger->info("Unable to proceed, unknown path. path={}", cwd.string());
    return {};
  }

  std::vector<std::string> file_names;

  for (const auto &entry : fs::directory_iterator(cwd))
  {
    if (entry.is_regular_file())
    {
      if (const auto s = fs::absolute(entry.path()).string(); s.ends_with(extension)) {
        fmt::print("entry: {}\n", s);
        file_names.emplace_back(s);
      }
    }
  }

  return file_names;
}
