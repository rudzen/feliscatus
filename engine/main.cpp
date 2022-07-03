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
#include <spdlog/spdlog.h>

#include "../src/bitboard.hpp"
#include "../src/board.hpp"
#include "../src/uci.hpp"
#include "../src/transpositional.hpp"
#include "../src/polyglot.hpp"
#include "../io/directory_resolver.hpp"
#include "../io/settings_resolver.hpp"

int main(const int argc, char *argv[])
{
  util::check_size<PawnHashEntry, 32>();

  spdlog::flush_every(std::chrono::seconds(3));

  const auto info = misc::print_engine_info<false>();

  fmt::print("{}", info);

  bitboard::init();
  Board::init();

  auto f = directory_resolver::get_book_list(Settings::settings.books_directory());

  if (!f.empty())
    fmt::print("info string Detected {} books\n", f.size());

  uci::init(Options, f);

  TT.init(1);

  uci::run(argc, argv);
}