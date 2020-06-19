#include <string>
#include <string_view>
#include <memory>
#include <chrono>
#include <sstream>
#include <fmt/format.h>
#include <CLI/CLI.hpp>
#include "tune.h"
#include "../src/square.h"
#include "../src/bitboard.h"
#include "../src/magic.h"
#include "../src/zobrist.h"
#include "../src/transpositional.h"
#include "file_resolver.h"
#include "cli_parser.h"

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

  auto cli_parser_settings = cli::make_parser(argc, argv, title, ParserType::Tuner);

  TT.init(256);
  squares::init();
  bitboard::init();
  attacks::init();
  zobrist::init();

  const Stopwatch sw;
  eval::Tune(std::make_unique<Game>(), cli_parser_settings.get());
  const auto seconds = sw.elapsed_seconds();
  fmt::print("{} seconds\n", seconds);
}