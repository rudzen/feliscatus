#pragma once

#include <thread>
#include <memory>
#include <vector>
#include <mutex>
#include "pawnhashtable.h"

class Game;
class Search;

struct Worker {

  Worker(std::size_t index);
  virtual ~Worker();
  void start(Game *master);

  void stop();

  std::unique_ptr<Search> search_{};

private:
  std::size_t index_;
  PawnHashTable pawn_hash_{};
  std::unique_ptr<Game> game_{};
  std::jthread thread_{};
  std::mutex mutex_;
};

struct WorkerPool final : std::vector<Worker*> {

  using vector<Worker*>::vector;

  Worker* &operator[](const int i) { return vector<Worker*>::at(i); }

  void set(std::size_t v);

  void stop();

  [[nodiscard]]
  uint64_t node_count() const;

};

inline WorkerPool Pool{};