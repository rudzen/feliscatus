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

#pragma once

#include <array>
#include <iostream>
#include <string>
#include <map>
#include <span>

#include <fmt/format.h>

#include "miscellaneous.hpp"
#include "types.hpp"
#include "pv_entry.hpp"
#include "search_limits.hpp"
#include "cpu.hpp"

struct Felis;
struct Board;

namespace uci {

inline Cpu CpuLoad;

enum class UciOptions {
  THREADS,
  HASH,
  HASH_X_THREADS,
  CLEAR_HASH,
  CLEAR_HASH_NEW_GAME,
  PONDER,
  UCI_Chess960,
  SHOW_CPU,
  UCI_OPT_NB = 8
};

using uci_t = std::underlying_type_t<UciOptions>;

constexpr std::array<std::string_view, static_cast<uci_t>(UciOptions::UCI_OPT_NB)> UciStrings
{
  "Threads",
  "Hash",
  "Hash * Threads",
  "Clear Hash",
  "Clear hash on new game",
  "Ponder",
  "UCI_Chess960",
  "Show CPU usage"
};

template<UciOptions Option>
[[nodiscard]]
constexpr std::string_view get_uci_name() {
  return UciStrings[static_cast<uci_t>(Option)];
}

class Option;

/// Custom comparator because UCI options should be case insensitive
struct CaseInsensitiveLess final {
  bool operator()(std::string_view, std::string_view) const noexcept;
};

/// Configure a std map to use as options container
using OptionsMap = std::map<std::string_view, Option, CaseInsensitiveLess>;

/// Option class implements an option as defined by UCI protocol
class [[nodiscard]] Option final {

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
  Option(const char *v, const char *cur, on_change = nullptr);

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
  std::string_view default_value() const noexcept;

  [[nodiscard]]
  std::string_view current_value() const noexcept;

  [[nodiscard]]
  std::string_view type() const noexcept;

  [[nodiscard]]
  int max() const noexcept;

  [[nodiscard]]
  int min() const noexcept;

private:
  std::string default_value_{};
  std::string current_value_{};
  std::string type_{};
  int min_{};
  int max_{};
  std::size_t idx_{};
  on_change on_change_{};
};

void init(OptionsMap &);

void post_moves(Move m, Move ponder_move);

void post_info(int d, int selective_depth);

void post_curr_move(Move m, int m_number);

void post_pv(int d, int max_ply, int score, const std::span<PVEntry> &pv_line, NodeType nt);

[[nodiscard]]
std::string display_uci(Move m);

[[nodiscard]]
std::string info(std::string_view info_string);

void run(int argc, char *argv[]);

}// namespace uci

inline uci::OptionsMap Options;

///
/// Options formatter
///
template<>
struct fmt::formatter<uci::OptionsMap> : formatter<std::string_view> {
  // parse is inherited from formatter<string_view>.
  template<typename FormatContext>
  auto format(const uci::OptionsMap om, FormatContext &ctx) {
    fmt::memory_buffer buffer;

    for (std::size_t idx = 0; idx < om.size(); ++idx)
      for (const auto &it : om)
      {
        if (it.second.index() != idx)
          continue;

        const auto &o = it.second;
        const auto type = o.type();
        const auto default_value = o.default_value();

        fmt::format_to(buffer, "\noption name {} type {}", it.first, type);

        if (type != "button")
          fmt::format_to(buffer, " default {}", default_value);

        if (type == "spin")
          fmt::format_to(buffer, " min {} max {}", o.min(), o.max());

        break;
      }

    return formatter<std::string_view>::format(fmt::to_string(buffer), ctx);
  }
};

///
/// Move formatter
///
template<>
struct fmt::formatter<Move> : formatter<std::string_view> {
  // parse is inherited from formatter<string_view>.
  template<typename FormatContext>
  auto format(const Move m, FormatContext &ctx) {
    return formatter<std::string_view>::format(uci::display_uci(m), ctx);
  }
};

///
/// Square formatter
///
template<>
struct fmt::formatter<Square> : formatter<std::string_view> {
  // parse is inherited from formatter<string_view>.
  template<typename FormatContext>
  auto format(const Square s, FormatContext &ctx) {
    return formatter<std::string_view>::format(SquareString[s], ctx);
  }
};

///
/// File formatter
///
template<>
struct fmt::formatter<File> : formatter<std::string_view> {
  // parse is inherited from formatter<string_view>.
  template<typename FormatContext>
  auto format(const File f, FormatContext &ctx) {
    return formatter<std::string_view>::format('a' + static_cast<char>(f), ctx);
  }
};