#include <docopt/docopt.h>
#include <string>
#include <string_view>
#include <memory>
#include "tune.h"


static constexpr auto title =
  R"(___    _ _     ___      _
    | __|__| (_)___/ __|__ _| |_ _  _ ___
    | _/ -_) | (_-< (__/ _` |  _| || (_-<
    |_|\___|_|_/__/\___\__,_|\__|\_,_/__/)
           | |_ _  _ _ _  ___ _ _
           |  _| || | ' \/ -_) '_|
            \__|\_,_|_||_\___|_|)";

static constexpr auto USAGE =
  R"(Mirage Simple Tuner.
    Usage:
          simple_tuner -h
          simple_tuner [--psqt] [--piecevalue] [--king] [--queen] [--rook] [--bishop] [--knight] [--pawn] [--passedpawn] [--coordination] [--centercontrol] [--tempo] [--space] [--mobility] [--attbypawn] [--limitadjust] [--lazymargin]

  Options:
          -h --help           Show this screen.

          --epd=FILE          Sets the file to read from [default: ./out.epd].
          --log=FILE          Prefix for logfile [default: _feliscatus_tuner].
          --psqt              Enable piece square table value tuning [default: true].
          --piecevalue        Enable piece value tuning [default: false].
          --king              Enable king evaluation tuning [default: false].
          --queen             Enable queen evaluation tuning [default: false].
          --rook              Enable rook evaluation tuning [default: false].
          --bishop            Enable bishop evaluation tuning [default: false].
          --knight            Enable knight evaluation tuning [default: false].
          --pawn              Enable pawn evaluation tuning [default: false].
          --passedpawn        Enable passed pawn evaluation tuning [default: false].
          --coordination      Enable piece coordination evaluation tuning [default: false].
          --centercontrol     Enable center control evaluation tuning [default: false].
          --tempo             Enable tempo evaluation tuning [default: false].
          --space             Enable space evaluation tuning [default: false].
          --mobility          Enable mobility evaluation tuning [default: false].
          --attbypawn         Enable attacked by pawn evaluation tuning [default: false].
          --weak              Enable weakness evaluation tuning [default: false].
          --limitadjust       Enable game limit adjusting evaluation tuning [default: false].
          --imbalance         Enable imbalance evaluation tuning [default: false].
)";

int main(int argc, char **argv) {

  std::map<std::string, docopt::value> args = docopt::docopt(USAGE, { std::next(argv), std::next(argv, argc) }, true, "Felis Catus Tuner");

  // get filename from command line arguments
  const auto input_file = args["--epd"].asString();
  const auto output_file = args["--log"].asString();

  auto game = std::make_unique<Game>();

  Stopwatch sw;
  eval::Tune(game.get(), input_file, output_file, args);
  const auto seconds = sw.elapsed_seconds();
  printf("%f\n", seconds);
}