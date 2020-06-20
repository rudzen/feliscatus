#pragma once

#include <memory>
#include <string>

enum ParserType { Tuner, Engine };

struct ParserSettings {
  std::string file_name{};
  std::string log_file_prefix{};
  bool pawn{};
  bool knight{};
  bool bishop{};
  bool rook{};
  bool queen{};
  bool king{};
  bool psqt{};
  bool coordination{};
  bool weakness{};
  bool strength{};
  bool mobility{};
  bool tempo{};
  bool lazy_margin{};
  bool passed_pawn{};
};

namespace cli {

std::unique_ptr<ParserSettings> make_parser(int argc, char **argv, const std::string &title, ParserType type);

}