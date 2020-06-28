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

#pragma once

#include <memory>
#include <vector>

#include "protocol.h"
#include "worker.h"

struct PawnHashTable;
struct Search;

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

  bool set_option(std::string_view name, std::string_view value) override;

  int run(int argc, char* argv[]);

private:
  std::unique_ptr<Game> game;
  std::unique_ptr<Search> search;
  std::unique_ptr<UCIProtocol> protocol;
  std::vector<Worker> workers{};
  uint64_t num_threads;
};
