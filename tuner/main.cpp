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

#include <string>
#include <string_view>
#include <memory>
#include <chrono>
#include <sstream>

#include <fmt/format.h>

#include "tune.h"
#include "../src/board.h"
#include "../src/square.h"
#include "../src/bitboard.h"
#include "../src/magic.h"
#include "../src/zobrist.h"
#include "../src/transpositional.h"
#include "../src/datapool.h"
#include "file_resolver.h"
#include "../cli/cli_parser.h"

namespace {

constexpr auto title =
  R"(
     ___    _ _     ___      _
    | __|__| (_)___/ __|__ _| |_ _  _ ___
    | _/ -_) | (_-< (__/ _` |  _| || (_-<
    |_|\___|_|_/__/\___\__,_|\__|\_,_/__/
           | |_ _  _ _ _  ___ _ _
           |  _| || | ' \/ -_) '_|
            \__|\_,_|_||_\___|_|)";

}// namespace

int main(const int argc, char **argv) {

  fmt::print("{}\n", title);

  const auto cli_parser_settings = cli::make_parser(argc, argv, title, ParserType::Tuner);

  TT.init(256);

  squares::init();
  bitboard::init();
  attacks::init();
  zobrist::init();

  const Stopwatch sw;
  eval::Tune(std::make_unique<Board>(), cli_parser_settings.get());
  const auto seconds = sw.elapsed_seconds();
  fmt::print("{} seconds\n", seconds);
}