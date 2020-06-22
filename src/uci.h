#pragma once

#include "protocol.h"
#include <fmt/format.h>

class Game;

struct UCIProtocol final : Protocol {
  UCIProtocol(ProtocolListener *cb, Game *g);

  void check_input() override;

  void post_moves(Move bestmove, Move pondermove) override;

  void post_info(int d, int selective_depth, uint64_t node_count, TimeUnit time, int hash_full) override;

  void post_curr_move(Move curr_move, int curr_move_number) override;

  void post_pv(int d, int max_ply, uint64_t node_count, TimeUnit time, int hash_full, int score, fmt::memory_buffer &pv, NodeType node_type) override;

  int handle_input(const char *params[], int num_params) override;

  int handle_go(const char *params[], int num_params, ProtocolListener *cb);

  int handle_position(const char *params[], int num_params) const;

  bool handle_set_option(const char *params[], int num_params) const;
};
