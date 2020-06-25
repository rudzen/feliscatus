#pragma once

#include <fmt/format.h>
#include "protocol.h"

class Game;

struct UCIProtocol final : Protocol {
  UCIProtocol(ProtocolListener *cb, Game *g);

  void post_moves(Move bestmove, Move pondermove) override;

  void post_info(int d, int selective_depth, uint64_t node_count, uint64_t nodes_per_sec, TimeUnit time, int hash_full) override;

  void post_curr_move(Move curr_move, int curr_move_number) override;

  void post_pv(int d, int max_ply, uint64_t node_count, uint64_t nodes_per_second, TimeUnit time, int hash_full, int score, fmt::memory_buffer &pv, NodeType node_type) override;

  int handle_go(std::istringstream& input);

  void handle_position(Game* g, std::istringstream& input) const;

  bool handle_set_option(std::istringstream& input) const;
};
