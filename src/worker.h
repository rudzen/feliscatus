#pragma once

#include <thread>
#include "game.h"
#include "eval.h"
#include "search.h"

struct Worker {
  void start(Game *master, PawnHash *pawnt) {
    game_ = new Game();
    game_->copy(master);
    eval_   = new Eval(*game_, pawnt);
    search_ = new Search(game_, eval_);
    thread_ = std::jthread(&Search::run, search_);
  }

  void stop() {
    search_->stop();
    thread_.request_stop();
    thread_.join();
    delete search_;
    delete eval_;
    delete game_;
  }

private:
  Game *game_;
  Eval *eval_;
  Search *search_;
  std::jthread thread_;
};