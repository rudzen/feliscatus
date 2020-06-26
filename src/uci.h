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

#include <fmt/format.h>

#include "protocol.h"

class Game;

struct UCIProtocol final : Protocol {
  UCIProtocol(ProtocolListener *cb, Game *g);

  void post_moves(Move bestmove, Move pondermove) override;

  void post_info(int d, int selective_depth, uint64_t node_count, uint64_t nodes_per_sec, TimeUnit time, int hash_full) override;

  void post_curr_move(Move curr_move, int curr_move_number) override;

  void post_pv(int d, int max_ply, uint64_t node_count, uint64_t nodes_per_second, TimeUnit time, int hash_full, int score, const std::array<PVEntry, MAXDEPTH> &pv, int pv_length, int ply, NodeType node_type) override;

  int handle_go(std::istringstream& input);

  void handle_position(std::istringstream& input) const;

  bool handle_set_option(std::istringstream& input) const;
};
