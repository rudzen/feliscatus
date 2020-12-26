#include <CLI/CLI.hpp>
#include "cli_parser.hpp"

struct CliParser final {
  CliParser(const int argc, char **argv, const std::string &title) : argc_(argc), argv_(argv), app_(title) {}

  template<ParserType T>
  [[nodiscard]]
  const ParserSettings &parse();

private:
  int argc_;
  char **argv_;
  CLI::App app_;
  ParserSettings parser_settings_{};
};

template<ParserType T>
const ParserSettings &CliParser::parse() {
  static_assert(T == Tuner || T == Engine);
  if constexpr (T == Tuner)
  {
    auto *const file_option = app_.add_option("-f,--file", parser_settings_.file_name, "The PGN file to read as input");
    auto *const pawn_option   = app_.add_flag("-p, --pawn", parser_settings_.pawn, "Enables pawn tuning.");
    auto *const pp_option     = app_.add_flag("--passedpawn", parser_settings_.passed_pawn, "Enables passed pawn tuning.");
    auto *const knight_option = app_.add_flag("-n, --knight", parser_settings_.knight, "Enables knight tuning.");
    auto *const bishop_option = app_.add_flag("-b, --bishop", parser_settings_.bishop, "Enables bishop tuning.");
    auto *const rook_option   = app_.add_flag("-r, --rook", parser_settings_.rook, "Enables rook tuning.");
    auto *const queen_option  = app_.add_flag("-q, --queen", parser_settings_.queen, "Enables queen tuning.");
    auto *const king_option   = app_.add_flag("-k, --king", parser_settings_.king, "Enables king tuning.");
    auto *const psqt_option   = app_.add_flag("--psqt", parser_settings_.psqt, "Enabled piece square value tuning");
    auto *const coord_option  = app_.add_flag("--coordination", parser_settings_.coordination, "Enables bishop pair tuning");
    auto *const str_option    = app_.add_flag("--strength", parser_settings_.strength, "Enable attack strength evaluation tuning");
    auto *const weak_option   = app_.add_flag("--weakness", parser_settings_.weakness, "Enable weakness evaluation tuning");
    auto *const mob_option    = app_.add_flag("--mobility", parser_settings_.mobility, "Enables piece mobility tuning");
    auto *const tempo_option  = app_.add_flag("--tempo", parser_settings_.tempo, "Enables tempo evaluation tuning");
    auto *const lazy_option   = app_.add_flag("--lazy_margin", parser_settings_.lazy_margin, "Enables lazy_margin cut-off tuning");

    file_option->required();
    pp_option->needs(pawn_option);
    psqt_option->needs(pawn_option, knight_option, bishop_option, rook_option, queen_option, king_option);
    coord_option->needs(bishop_option);
    str_option->needs(pawn_option, knight_option, bishop_option, rook_option, queen_option, king_option);
    weak_option->needs(pawn_option, knight_option, bishop_option, rook_option, queen_option, king_option);
    mob_option->needs(pawn_option, knight_option, bishop_option, rook_option, queen_option, king_option);

    try
    { app_.parse(argc_, argv_); } catch (const CLI::ParseError &e)
    { app_.exit(e); }

    parser_settings_.pawn         = !pawn_option->empty();
    parser_settings_.passed_pawn  = !pp_option->empty();
    parser_settings_.knight       = !knight_option->empty();
    parser_settings_.bishop       = !bishop_option->empty();
    parser_settings_.rook         = !rook_option->empty();
    parser_settings_.queen        = !queen_option->empty();
    parser_settings_.king         = !king_option->empty();
    parser_settings_.psqt         = !psqt_option->empty();
    parser_settings_.coordination = !coord_option->empty();
    parser_settings_.strength     = !str_option->empty();
    parser_settings_.weakness     = !weak_option->empty();
    parser_settings_.mobility     = !mob_option->empty();
    parser_settings_.tempo        = !tempo_option->empty();
    parser_settings_.lazy_margin  = !lazy_option->empty();

  } else if constexpr (T == Engine)
  {}

  return parser_settings_;
}

namespace cli {

std::unique_ptr<ParserSettings> make_parser(const int argc, char **argv, const std::string &title, const ParserType type) {

  if (type == Tuner)
    return std::make_unique<ParserSettings>(CliParser(argc, argv, title).parse<Tuner>());

  if (type == Engine)
    return std::make_unique<ParserSettings>(CliParser(argc, argv, title).parse<Engine>());

  exit(1);
}

}// namespace cli
