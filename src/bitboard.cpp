#include <string>
#include <string_view>

#include <fmt/format.h>

#include "bitboard.h"
#include "types.h"

std::string bitboard::print_bitboard(Bitboard b, std::string_view title) {

  fmt::memory_buffer buffer;

  constexpr std::string_view line = "+---+---+---+---+---+---+---+---+";

  if (!title.empty())
    fmt::format_to(buffer, "{}\n", title);

  fmt::format_to(buffer, "{}\n", line);

  for (const auto r : ReverseRanks)
  {
    for (const auto f : Files)
      fmt::format_to(buffer, "| {} ", b & make_square(f, r) ? "X" : " ");

    fmt::format_to(buffer, "| {}\n{}\n", std::to_string(1 + r), line);
  }
  fmt::format_to(buffer, "  a   b   c   d   e   f   g   h\n");

  return fmt::to_string(buffer);
}
