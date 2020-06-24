#pragma once

#include <thread>
#include <memory>
#include "game.h"
#include "eval.h"
#include "search.h"
#include "pawnhashtable.h"

struct Worker {

  void start(Game *master) {
    game_ = std::make_unique<Game>();
    game_->copy(master);
    search_ = std::make_unique<Search>(game_.get(), &pawn_hash_);
    thread_ = std::jthread(&Search::run, search_.get());
  }

  void stop() {
    if (search_)
      search_->stop();
    if (thread_.joinable())
    {
      thread_.request_stop();
      thread_.join();
    }
  }

private:
  PawnHashTable pawn_hash_{};
  std::unique_ptr<Game> game_{};
  std::unique_ptr<Search> search_{};
  std::jthread thread_{};
};