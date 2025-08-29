// Copyright (C) 2008-2016 Gunnar Harms (Bobcat author)
// Copyright (C) 2017      FireFather (Tomcat author)
// Copyright (C) 2020-2025 Rudy Alex Kohn
// See end of file for extended copyright information.

#include <CLI/CLI.hpp>
#include "cli_parser.hpp"

struct CliParser final {
  CliParser(const int argc, char **argv, const std::string &title) : m_argc(argc), m_argv(argv), m_app(title) {}

  template<ParserType T>
  [[nodiscard]]
  const ParserSettings &parse();

private:
  int m_argc;
  char **m_argv;
  CLI::App m_app;
  ParserSettings m_parser_settings{};
};

template<ParserType T>
const ParserSettings &CliParser::parse() {
  static_assert(T == Tuner || T == Engine);
  if constexpr (T == Tuner)
  {
    auto *const fileOption =
      m_app.add_option("-f,--file", m_parser_settings.file_name, "The PGN file to read as input");
    auto *const pawnOption = m_app.add_flag("-p, --pawn", m_parser_settings.pawn, "Enables pawn tuning.");
    auto *const ppOption = m_app.add_flag("--passedpawn", m_parser_settings.passed_pawn, "Enables passed pawn tuning.");
    auto *const knightOption  = m_app.add_flag("-n, --knight", m_parser_settings.knight, "Enables knight tuning.");
    auto *const bishopOption  = m_app.add_flag("-b, --bishop", m_parser_settings.bishop, "Enables bishop tuning.");
    auto *const rookOption    = m_app.add_flag("-r, --rook", m_parser_settings.rook, "Enables rook tuning.");
    auto *const queenOption   = m_app.add_flag("-q, --queen", m_parser_settings.queen, "Enables queen tuning.");
    auto *const kingOption    = m_app.add_flag("-k, --king", m_parser_settings.king, "Enables king tuning.");
    auto *const psqtOption    = m_app.add_flag("--psqt", m_parser_settings.psqt, "Enabled piece square value tuning");
    auto *const coordOption =
      m_app.add_flag("--coordination", m_parser_settings.coordination, "Enables bishop pair tuning");
    auto *const strOption =
      m_app.add_flag("--strength", m_parser_settings.strength, "Enable attack strength evaluation tuning");
    auto *const weakOption =
      m_app.add_flag("--weakness", m_parser_settings.weakness, "Enable weakness evaluation tuning");
    auto *const mobOption = m_app.add_flag("--mobility", m_parser_settings.mobility, "Enables piece mobility tuning");
    auto *const tempoOption = m_app.add_flag("--tempo", m_parser_settings.tempo, "Enables tempo evaluation tuning");
    auto *const lazyOption =
      m_app.add_flag("--lazy_margin", m_parser_settings.lazy_margin, "Enables lazy_margin cut-off tuning");

    fileOption->required();
    ppOption->needs(pawnOption);
    psqtOption->needs(pawnOption, knightOption, bishopOption, rookOption, queenOption, kingOption);
    coordOption->needs(bishopOption);
    strOption->needs(pawnOption, knightOption, bishopOption, rookOption, queenOption, kingOption);
    weakOption->needs(pawnOption, knightOption, bishopOption, rookOption, queenOption, kingOption);
    mobOption->needs(pawnOption, knightOption, bishopOption, rookOption, queenOption, kingOption);

    try
    {
      m_app.parse(m_argc, m_argv); } catch (const CLI::ParseError &e)
    {
      m_app.exit(e); }

    m_parser_settings.pawn         = !pawnOption->empty();
    m_parser_settings.passed_pawn  = !ppOption->empty();
    m_parser_settings.knight       = !knightOption->empty();
    m_parser_settings.bishop       = !bishopOption->empty();
    m_parser_settings.rook         = !rookOption->empty();
    m_parser_settings.queen        = !queenOption->empty();
    m_parser_settings.king         = !kingOption->empty();
    m_parser_settings.psqt         = !psqtOption->empty();
    m_parser_settings.coordination = !coordOption->empty();
    m_parser_settings.strength     = !strOption->empty();
    m_parser_settings.weakness     = !weakOption->empty();
    m_parser_settings.mobility     = !mobOption->empty();
    m_parser_settings.tempo        = !tempoOption->empty();
    m_parser_settings.lazy_margin  = !lazyOption->empty();

  } else if constexpr (T == Engine)
  {}

  return m_parser_settings;
}

namespace cli {

std::unique_ptr<ParserSettings> makeParser(const int argc, char **argv, const std::string &title, const ParserType type) {

  if (type == Tuner)
    return std::make_unique<ParserSettings>(CliParser(argc, argv, title).parse<Tuner>());

  if (type == Engine)
    return std::make_unique<ParserSettings>(CliParser(argc, argv, title).parse<Engine>());

  exit(1);
}

}// namespace cli

// Feliscatus, a UCI chess playing engine derived from Tomcat 1.0 (Bobcat 8.0)
// Copyright (C) 2008-2016 Gunnar Harms (Bobcat author)
// Copyright (C) 2017      FireFather (Tomcat author)
// Copyright (C) 2020-2022 Rudy Alex Kohn
//
// Feliscatus is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Feliscatus is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Feliscatus.  If not, see <http://www.gnu.org/licenses/>.