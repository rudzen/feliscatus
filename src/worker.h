#pragma once

#include <thread>

struct Worker {
  void start(Game *master, PawnHash *pawnt) {
    game_ = new Game();
    game_->copy(master);
    see_    = new See(game_);
    eval_   = new Eval(*game_, pawnt);
    search_ = new Search(game_, eval_, see_);
    thread_ = std::jthread(&Search::run, search_);
  }

  void stop() {
    search_->stop();
    thread_.request_stop();
    thread_.join();
    delete search_;
    delete eval_;
    delete see_;
    delete game_;
  }

private:
  Game *game_;
  Eval *eval_;
  See *see_;
  Search *search_;
  std::jthread thread_;
};