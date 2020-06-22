#pragma once

#include <thread>
#include <memory>
#include "search.h"
#include "pawnhashtable.h"
#include "game.h"

struct Worker {

  Worker(const std::size_t index) : index_(index) { }

  void start(Game *master) {
    game_ = std::make_unique<Game>();
    game_->copy(master);
    search_ = std::make_unique<Search>(game_.get(), &pawn_hash_);
    thread_ = std::jthread(&Search::run, search_.get());
  }

  void stop() {
    search_->stop();
    thread_.request_stop();
    thread_.join();
  }

private:
  std::size_t index_;
  PawnHashTable pawn_hash_{};
  std::unique_ptr<Game> game_{};
  std::unique_ptr<Search> search_{};
  std::jthread thread_{};
};

struct WorkerPool final : std::vector<std::unique_ptr<Worker>> {

  using vector<std::unique_ptr<Worker>>::vector;

  std::unique_ptr<Worker> &operator[](const int i) { return vector<std::unique_ptr<Worker>>::at(i); }// range-checked

  const std::unique_ptr<Worker> &operator[](const int i) const { return vector<std::unique_ptr<Worker>>::at(i); }

  void set(std::size_t v);

  void stop();


};

inline WorkerPool Pool{};