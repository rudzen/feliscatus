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

constexpr int FIXED_MOVE_TIME    = 1;
constexpr int FIXED_DEPTH        = 2;
constexpr int INFINITE_MOVE_TIME = 4;
constexpr int PONDER_SEARCH      = 8;

struct SearchLimits {
  std::array<TimeUnit, COL_NB> time{};
  std::array<TimeUnit, COL_NB> inc{};
  TimeUnit movetime{};
  int movestogo{};
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

  virtual void check_input()                                      = 0;

  virtual void post_moves(Move bestmove, Move pondermove)         = 0;

  virtual void post_info(int depth, int selective_depth, uint64_t node_count, TimeUnit time, int hash_full) = 0;

  virtual void post_curr_move(Move curr_move, int curr_move_number) = 0;

  virtual void post_pv(int depth, int max_ply, uint64_t node_count, TimeUnit time, int hash_full, int score, fmt::memory_buffer &pv, NodeType node_type) = 0;

  [[nodiscard]]
  int is_analysing() const noexcept { return flags & (INFINITE_MOVE_TIME | PONDER_SEARCH); }

  [[nodiscard]]
  int is_fixed_time() const noexcept { return flags & FIXED_MOVE_TIME; }

  [[nodiscard]]
  int is_fixed_depth() const noexcept { return flags & FIXED_DEPTH; }

  [[nodiscard]]
  int get_depth() const noexcept { return depth; }

  void set_flags(const int f) noexcept { this->flags = f; }

protected:
  int flags{};
  int depth{};
  ProtocolListener *callback;
  Game *game;
};
