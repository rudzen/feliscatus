#pragma once

#include <thread>
#include <memory>
#include "game.h"
#include "eval.h"
#include "search.h"
#include "hash.h"

struct Worker {

  Worker() : pawn_hash_(8) {}

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
  PawnHash pawn_hash_;
  std::unique_ptr<Game> game_{};
  std::unique_ptr<Search> search_{};
  std::jthread thread_{};
};