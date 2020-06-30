/*
  Feliscatus, a UCI chess playing engine derived from Tomcat 1.0 (Bobcat 8.0)
  Copyright (C) 2008-2016 Gunnar Harms (Bobcat author)
  Copyright (C) 2017      FireFather (Tomcat author)
  Copyright (C) 2020      Rudy Alex Kohn

  Feliscatus is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Feliscatus is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <string_view>
#include <algorithm>
#include <memory>
#include <thread>
#include <iostream>
#include <sstream>

#include <fmt/format.h>

#include "feliscatus.h"
#include "perft.h"
#include "util.h"
#include "types.h"
#include "datapool.h"

int Felis::new_game() {
  const auto num_threads = static_cast<std::size_t>(Options[uci::get_uci_name<uci::UciOptions::THREADS>()]);
  Pool.set(num_threads);
  if (const auto num_helpers = num_threads - 1; num_helpers == 0)
    workers.clear();
  else
    workers.resize(num_helpers);
  return game->new_game(Game::kStartPosition.data());
}

int Felis::set_fen(const std::string_view fen) { return game->new_game(fen); }

int Felis::go() {
  game->pos->pv_length = 0;

  go_search(Pool.limits);

  if (game->pos->pv_length)
  {
    auto &pv = Pool.main()->pv;
    const auto [move, ponder_move] = std::make_pair(pv[0][0].move, pv[0][1].move ? pv[0][1].move : MOVE_NONE);
    uci::post_moves(move, ponder_move);
    game->make_move(move, true, true);
  }
  return 0;
}

void Felis::stop() { search->stop_search.store(true); }

bool Felis::make_move(const std::string_view m) const {
  const auto *const move = game->pos->string_to_move(m);
  return move ? game->make_move(*move, true, true) : false;
}

void Felis::go_search(SearchLimits &limits) {
  start_workers();
  search->go(limits);
  stop_workers();
}

void Felis::start_workers() {
  if (workers.empty())
    return;
  const auto fen = game->get_fen();
  for (std::size_t idx = 1; auto &worker : workers)
    worker.start(fen, idx++);
}

void Felis::stop_workers() {
  for (auto &worker : workers)
    worker.stop();
}

int Felis::run(const int argc, char* argv[]) {
  setbuf(stdout, nullptr);

  game     = std::make_unique<Game>();
  Pool.set(1);
  search   = std::make_unique<Search>(game.get(), 0);

  new_game();

  // simple jthread to start main search from
  std::jthread main_go;

  const auto stop_threads = [&]() {
      stop();
      stop_workers();
      if (main_go.joinable())
        main_go.join();
  };

  std::string command;
  std::string token;

  // TODO : replace with CLI
  for (auto argument_index = 1; argument_index < argc; ++argument_index)
    command += fmt::format("{} ", argv[argument_index]);

  do
  {
    if (argc == 1 && !std::getline(std::cin, command))
      command = "quit";
    else
      game->pos->generate_moves();

    std::istringstream input(command);

    token.clear();
    input >> std::skipws >> token;

    if (token == "quit" || token == "stop")
      stop_threads();
    else if (token == "ponder")
      Pool.time.ponder_hit();
    else if (token == "uci")
    {
      fmt::print("id name Feliscatus 0.1\n");
      fmt::print("id author Gunnar Harms, FireFather, Rudy Alex Kohn\n");
      fmt::print("{}\n", Options);
      fmt::print("uciok\n");
    } else if (token == "isready")
      fmt::print("readyok\n");
    else if (token == "ucinewgame")
    {
      if (Options[uci::get_uci_name<uci::UciOptions::CLEAR_HASH_NEW_GAME>()])
        TT.clear();
      new_game();
      fmt::print("readyok\n");
    } else if (token == "setoption")
      uci::handle_set_option(input);
    else if (token == "position")
      uci::handle_position(game.get(), input);
    else if (token == "go")
    {
      stop_threads();
      uci::handle_go(input, Pool.limits);
      main_go = std::jthread(&Felis::go, this);
    }
    else if (token == "perft")
    {
      const auto total = perft::perft(game.get(), 6);
      fmt::print("Total nodes: {}\n", total);
    }
    else if (token == "quit" || token == "exit")
      break;
  } while (token != "quit" && argc == 1);

  stop_threads();

  return 0;
}
