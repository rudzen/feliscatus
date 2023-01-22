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

#pragma once

#include <algorithm>
#include <array>
#include <filesystem>
#include <iostream>
#include <map>
#include <string>
#include <span>

#include <fmt/format.h>

#include "miscellaneous.hpp"
#include "types.hpp"
#include "pv_entry.hpp"
#include "search_limits.hpp"
#include "cpu.hpp"

struct Board;

namespace uci
{

inline CpuLoad Cpu;

enum class UciOptions
{
  THREADS,
  HASH,
  HASH_X_THREADS,
  CLEAR_HASH,
  CLEAR_HASH_NEW_GAME,
  PONDER,
  UCI_Chess960,
  SHOW_CPU,
  USE_BOOK,
  BOOKS,
  BOOK_BEST_MOVE,
  UCI_OPT_NB = 11
};

using uci_t = std::underlying_type_t<UciOptions>;

template<UciOptions Option>
[[nodiscard]]
constexpr std::string_view uci_name()
{
  constexpr std::array<std::string_view, static_cast<uci_t>(UciOptions::UCI_OPT_NB)> UciStrings{
    "Threads",      "Hash",           "Hash * Threads", "Clear Hash", "Clear hash on new game", "Ponder",
    "UCI_Chess960", "Show CPU usage", "Use book",       "Books",      "Best Book Move"};

  return UciStrings[static_cast<uci_t>(Option)];
}

class Option;

enum class OptionType
{
  String,
  Check,
  Button,
  Spin,
  Combo
};

using option_type_t = std::underlying_type_t<OptionType>;

/// Custom comparator because UCI options should be case insensitive
struct CaseInsensitiveLess final
{
  bool operator()(std::string_view, std::string_view) const noexcept;
};

/// Configure a std map to use as options container
using OptionsMap = std::map<std::string_view, Option, CaseInsensitiveLess>;

/// Option class implements an option as defined by UCI protocol
class [[nodiscard]] Option final
{

  typedef void (*on_change)(const Option &);

public:
  [[nodiscard]]
  Option(on_change = nullptr);

  [[nodiscard]]
  Option(bool v, on_change = nullptr);

  [[nodiscard]]
  Option(const char *v, on_change = nullptr);

  [[nodiscard]]
  Option(int v, int minv, int maxv, on_change = nullptr);

  [[nodiscard]]
  Option(std::span<std::string> variants, const char *cur, on_change = nullptr);

  Option &operator=(const std::string &) noexcept;

  void operator<<(const Option &);

  [[nodiscard]]
  operator int() const;

  [[nodiscard]]
  operator std::string_view() const;

  [[nodiscard]]
  bool operator==(const char *) const;

  [[nodiscard]]
  std::size_t index() const noexcept;

  [[nodiscard]]
  std::span<std::string> variants() const noexcept;

  [[nodiscard]]
  std::string_view default_value() const noexcept;

  [[nodiscard]]
  std::string_view current_value() const noexcept;

  [[nodiscard]]
  OptionType type() const noexcept;

  [[nodiscard]]
  int max() const noexcept;

  [[nodiscard]]
  int min() const noexcept;

private:
  std::span<std::string> variants_;
  std::string default_value_{};
  std::string current_value_{};
  OptionType type_{};
  int min_{};
  int max_{};
  std::size_t idx_{};
  on_change on_change_{};
};

void init(OptionsMap &, std::span<std::string>);

void post_moves(Move m, Move ponder_move);

void post_info(int d, int selective_depth);

void post_curr_move(Move m, int m_number);

void post_pv(int d, int max_ply, int score, const std::span<PVEntry> &pv_line, NodeType nt);

[[nodiscard]]
std::string display_uci(Move m);

[[nodiscard]]
std::string info(const std::string_view info_string);

void run(int argc, char *argv[]);

}   // namespace uci

constinit inline uci::OptionsMap Options;

///
/// Options formatter
///
template<>
struct fmt::formatter<uci::OptionsMap> : formatter<std::string_view>
{
  // parse is inherited from formatter<string_view>.
  template<typename FormatContext>
  auto format(const uci::OptionsMap om, FormatContext &ctx)
  {
    static constexpr std::array<std::string_view, 5> Types{"string", "check", "button", "spin", "combo"};
    fmt::memory_buffer buffer;
    auto inserter = std::back_inserter(buffer);

    for (std::size_t idx = 0; idx < om.size(); ++idx)
      for (const auto &it : om)
      {
        if (it.second.index() != idx)
          continue;

        const auto &o            = it.second;
        const auto type          = o.type();
        const auto default_value = o.default_value();

        fmt::format_to(inserter, "\noption name {} type {} ", it.first, Types[static_cast<uci::option_type_t>(type)]);

        if (type != uci::OptionType::Button && type != uci::OptionType::Combo)
          fmt::format_to(inserter, "default {}", default_value);

        if (type == uci::OptionType::Spin)
          fmt::format_to(inserter, "min {} max {}", o.min(), o.max());

        if (type == uci::OptionType::Combo)
        {
          namespace fs = std::filesystem;

          fmt::format_to(inserter, "default {}", fs::path(o.current_value()).filename().string());

          std::for_each(o.variants().begin(), o.variants().end(), [&inserter](const auto v) {
            fmt::format_to(inserter, " var {}", fs::path(v).filename().string());
          });
        }

        break;
      }

    return formatter<std::string_view>::format(fmt::to_string(buffer), ctx);
  }
};

template<>
struct fmt::formatter<Move> : formatter<std::string_view>
{
  template<typename FormatContext>
  auto format(const Move m, FormatContext &ctx)
  {
    return formatter<std::string_view>::format(uci::display_uci(m), ctx);
  }
};

template<>
struct fmt::formatter<Square> : formatter<std::string_view>
{
  template<typename FormatContext>
  auto format(const Square s, FormatContext &ctx)
  {
    return formatter<std::string_view>::format(SquareString[s], ctx);
  }
};

template<>
struct fmt::formatter<File> : formatter<std::string_view>
{
  template<typename FormatContext>
  auto format(const File f, FormatContext &ctx)
  {
    return formatter<std::string_view>::format('a' + static_cast<char>(f), ctx);
  }
};
