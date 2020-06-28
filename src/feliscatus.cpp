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

namespace {

constexpr std::array<std::string_view, 2> on_off{"OFF", "ON"};

}

Felis::Felis()
  : num_threads(1) {
}

int Felis::new_game() {
  // TODO : test effect of not clearing
  //pawnt->clear();
  //TT.clear();
  return game->new_game(Game::kStartPosition.data());
}

int Felis::set_fen(const std::string_view fen) { return game->new_game(fen); }

int Felis::go(const SearchLimits &limits) {
  game->pos->pv_length = 0;

  if (game->pos->pv_length == 0)
    go_search(limits);

  if (game->pos->pv_length)
  {
    protocol->post_moves(search->pv[0][0].move, game->pos->pv_length > 1 ? search->pv[0][1].move : MOVE_NONE);
    game->make_move(search->pv[0][0].move, true, true);
  }
  return 0;
}

void Felis::ponder_hit() { search->search_time += search->start_time.elapsed_milliseconds(); }

void Felis::stop() { search->stop_search.store(true); }

bool Felis::make_move(const std::string_view m) const {
  const auto *const move = game->pos->string_to_move(m);
  return move ? game->make_move(*move, true, true) : false;
}

void Felis::go_search(const SearchLimits &limits) {
  start_workers();
  search->go(limits, num_threads);
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

bool Felis::set_option(const std::string_view name, const std::string_view value) {
  if (name == "Hash")
  {
    constexpr uint64_t min = 8;
    constexpr uint64_t max = 64 * 1024;
    const auto val     = util::to_integral<uint64_t>(value);
    TT.init(std::clamp(val, min, max));
    fmt::print("info string Hash:{}\n", TT.get_size_mb());
  } else if (name == "Threads" || name == "NumThreads")
  {
    constexpr std::size_t min = 1;
    constexpr std::size_t max = 64;
    const auto val     = util::to_integral<std::size_t>(value);
    num_threads = std::clamp(val, min, max);
    Pool.set(num_threads);
    workers.resize(num_threads - 1);
    workers.shrink_to_fit();
    fmt::print("info string Threads:{}\n", num_threads);
  } else if (name == "UCI_Chess960")
  {
    game->chess960 = value == "true";
    fmt::print("info string UCI_Chess960:{}\n", on_off[game->chess960]);
  } else if (name == "UCI_Chess960_Arena")
  {
    game->chess960 = game->xfen = value == "true";
    fmt::print("info string UCI_Chess960_Arena:{}\n", on_off[game->chess960]);
  } else
  {
    fmt::print("Unknown option. {}={}\n", name, value);
    return false;
  }

  return true;
}

int Felis::run(const int argc, char* argv[]) {
  setbuf(stdout, nullptr);

  Pool.set(1);
  game     = std::make_unique<Game>();
  protocol = std::make_unique<UCIProtocol>(this, game.get());
  // pawnt    = std::make_unique<PawnHashTable>();
  search   = std::make_unique<Search>(protocol.get(), game.get(), 0);

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
    command += std::string(argv[argument_index]) + ' ';

  do
  {
    if (argc == 1 && !std::getline(std::cin, command))
      command = "quit";
    // else
    //   game->pos->generate_moves();

    std::istringstream input(command);

    token.clear();
    input >> std::skipws >> token;

    if (token == "quit" || token == "stop")
      break;
    else if (token == "ponder")
      ponder_hit();
    else if (token == "uci")
    {
      fmt::print("id name Feliscatus 0.1\n");
      fmt::print("id author Gunnar Harms, FireFather, Rudy Alex Kohn\n");
      fmt::print("option name Hash type spin default 1024 min 8 max 65536\n");
      fmt::print("option name Ponder type check default true\n");
      fmt::print("option name Threads type spin default 1 min 1 max 64\n");
      fmt::print("option name UCI_Chess960 type check default false\n");
      fmt::print("uciok\n");
    } else if (token == "isready")
      fmt::print("readyok\n");
    else if (token == "ucinewgame")
    {
      new_game();
      fmt::print("readyok\n");
    } else if (token == "setoption")
      protocol->handle_set_option(input);
    else if (token == "position")
      protocol->handle_position(input);
    else if (token == "go")
    {
      protocol->handle_go(input);
      stop_threads();
      main_go = std::jthread(&Felis::go, this, protocol->limits);
    }
    else if (token == "quit" || token == "exit")
      break;
  } while (token != "quit" && argc == 1);

  stop_threads();

  return 0;
}
