#include <string>
#include <string_view>
#include <memory>
#include <fmt/format.h>
#include <docopt/docopt.h>
#include "tune.h"
#include "../src/square.h"
#include "../src/bitboard.h"
#include "../src/magic.h"
#include "../src/zobrist.h"
#include "../src/transpositional.h"

static constexpr auto title =
  R"(
     ___    _ _     ___      _
    | __|__| (_)___/ __|__ _| |_ _  _ ___
    | _/ -_) | (_-< (__/ _` |  _| || (_-<
    |_|\___|_|_/__/\___\__,_|\__|\_,_/__/
           | |_ _  _ _ _  ___ _ _
           |  _| || | ' \/ -_) '_|
            \__|\_,_|_||_\___|_|)";

static constexpr auto USAGE =
  R"(FelisCatus Tuner.
    Usage:
          FeliscatusTuner -h
          FeliscatusTuner [--pgn=FILE] [--log=FILE] [--pawn] [--knight] [--bishop] [--rook] [--queen] [--king] [--psqt] [--mobility] [--passedpawn] [--coordination] [--strength] [--weakness] [--tempo] [--lazy_margin]

  Options:
          -h --help           Show this screen.

          --pgn=FILE          Sets the file to read from [default: ./out.pgn].
          --log=FILE          Prefix for logfile [default: _feliscatus_tuner].
          --pawn              Enable pawn evaluation tuning [default: false].
          --knight            Enable knight evaluation tuning [default: false].
          --bishop            Enable bishop evaluation tuning [default: false].
          --rook              Enable rook evaluation tuning [default: false].
          --queen             Enable queen evaluation tuning [default: false].
          --king              Enable king evaluation tuning [default: false].
          --psqt              Enable piece square table value tuning (requires any piece) [default: false].
          --mobility          Enable mobility evaluation tuning (requires any piece) [default: false].
          --passedpawn        Enable passed pawn evaluation tuning (requires pawn) [default: false].
          --coordination      Enable piece coordination evaluation tuning (requires bishop) [default: false].
          --strength          Enable attack strength evaluation tuning (requires any piece) [default: false].
          --weakness          Enable weakness evaluation tuning (requires any piece) [default: false].
          --tempo             Enable tempo evaluation tuning [default: false].
          --lazy_margin       Enable evaluation lazy_margin tuning [default: false].
)";

int main(int argc, char **argv) {

  fmt::print("{}\n", title);

  std::map<std::string, docopt::value> args = docopt::docopt(USAGE, { std::next(argv), std::next(argv, argc) }, true, "Felis Catus Tuner");

  squares::init();
  bitboard::init();
  attacks::init();
  zobrist::init();

  TT.init(256);

  // get filename from command line arguments
  const auto input_file = "D:\\Chess\\Felis_self_play#1.at.pgn";//  args["--epd"].asString();
  const auto output_file = "D:\\Chess\\out.txt";// args["--log"].asString();

  auto game = std::make_unique<Game>();

  Stopwatch sw;
  eval::Tune(game.get(), input_file, output_file, args);
  const auto seconds = sw.elapsed_seconds();
  fmt::print("{} seconds\n", seconds);
}