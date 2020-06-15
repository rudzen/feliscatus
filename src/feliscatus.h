#pragma once

#include <memory>
#include <vector>
#include "protocol.h"
#include "worker.h"

struct PawnHashTable;
class Search;

struct Felis final : public ProtocolListener {
  Felis();

  int new_game() override;

  int set_fen(std::string_view fen) override;

  int go(int wtime = 0, int btime = 0, int movestogo = 0, int winc = 0, int binc = 0, int movetime = 5000) override;

  void ponder_hit() override;

  void stop() override;

  bool make_move(std::string_view m) const;

  void go_search(int wtime, int btime, int movestogo, int winc, int binc, int movetime);

  void start_workers();

  void stop_workers();

  int set_option(std::string_view name, std::string_view value) override;

  int run();

private:
  std::unique_ptr<Game> game;
  std::unique_ptr<Search> search;
  std::unique_ptr<Protocol> protocol;
  std::unique_ptr<PawnHashTable> pawnt;
  std::vector<Worker> workers{};
  std::size_t num_threads;
};
