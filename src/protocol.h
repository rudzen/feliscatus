#pragma once
#include <cstdint>
#include <string_view>
#include <array>
#include <fmt/format.h>
#include "stopwatch.h"
#include "types.h"

enum NodeType : uint8_t;
enum Move : uint32_t;
class Game;

constexpr auto static FenPieceNames = std::array<char, 16> {"PNBRQK  pnbrqk "};

struct SearchLimits {
  std::array<TimeUnit, COL_NB> time{};
  std::array<TimeUnit, COL_NB> inc{};
  TimeUnit movetime{};
  int movestogo{};
  int depth{};
  bool ponder{};
  bool infinite{};
  bool fixed_movetime{};
  bool fixed_depth{};

  void clear() {
    time.fill(0);
    inc.fill(0);
    movetime          = 0;
    movestogo = depth = 0;
    ponder = infinite = fixed_movetime = fixed_depth = false;
  }
};

struct ProtocolListener {
  virtual ~ProtocolListener()                                           = default;
  virtual int new_game()                                                = 0;
  virtual int set_fen(std::string_view fen)                             = 0;
  virtual int go(const SearchLimits &limits)                            = 0;
  virtual void ponder_hit()                                             = 0;
  virtual void stop()                                                   = 0;
  virtual int set_option(std::string_view name, std::string_view value) = 0;
};

struct Protocol {
  Protocol(ProtocolListener *cb, Game *g) : callback(cb), game(g) {}

  virtual ~Protocol() = default;

  virtual int handle_input(const char *params[], int num_params)  = 0;

  virtual void post_moves(Move bestmove, Move pondermove)         = 0;

  virtual void post_info(int depth, int selective_depth, uint64_t node_count, uint64_t nodes_per_sec, TimeUnit time, int hash_full) = 0;

  virtual void post_curr_move(Move curr_move, int curr_move_number) = 0;

  virtual void post_pv(int depth, int max_ply, uint64_t node_count, uint64_t nodes_per_second, TimeUnit time, int hash_full, int score, fmt::memory_buffer &pv, NodeType node_type) = 0;

  [[nodiscard]]
  bool is_analysing() const noexcept { return limits.infinite | limits.ponder; }

  [[nodiscard]]
  bool is_fixed_time() const noexcept { return limits.fixed_movetime; }

  [[nodiscard]]
  bool is_fixed_depth() const noexcept { return limits.fixed_depth; }

  [[nodiscard]]
  int get_depth() const noexcept { return limits.depth; }

  SearchLimits limits{};

protected:
  ProtocolListener *callback;
  Game *game;
};

inline std::string display_uci(const Move m) {

  if (m == MOVE_NONE)
    return "0000";

  // append piece promotion if the move is a promotion.
  return !is_promotion(m)
       ? fmt::format("{}{}", move_from(m), move_to(m))
       : fmt::format("{}{}{}", move_from(m), move_to(m), FenPieceNames[move_promoted(m) & 7]);
}

///
/// Move formatter
///
template<>
struct fmt::formatter<Move> : formatter<std::string_view> {
  // parse is inherited from formatter<string_view>.
  template<typename FormatContext>
  auto format(const Move c, FormatContext &ctx) {
    return formatter<std::string_view>::format(display_uci(c), ctx);
  }
};

///
/// Square formatter
///
template<>
struct fmt::formatter<Square> : formatter<std::string_view> {
  // parse is inherited from formatter<string_view>.
  template<typename FormatContext>
  auto format(const Square sq, FormatContext &ctx) {
    return formatter<std::string_view>::format(SquareString[sq], ctx);
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
