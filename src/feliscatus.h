#pragma once

#include <memory>
#include <vector>
#include "protocol.h"
#include "game.h"
#include "search.h"

struct PawnHashTable;

struct Felis final : public ProtocolListener {
  Felis();

  int new_game() override;

  int set_fen(std::string_view fen) override;

  int go(const SearchLimits &limits) override;

  void ponder_hit() override;

  void stop() override;

  bool make_move(std::string_view m) const;

  void go_search(const SearchLimits &limits);

  void start_workers();

  void stop_workers();

  int set_option(std::string_view name, std::string_view value) override;

  int run();

private:
  std::unique_ptr<Game> game;
  std::unique_ptr<Search> search;
  std::unique_ptr<Protocol> protocol;
  std::unique_ptr<PawnHashTable> pawnt;
  std::size_t num_threads;
};
