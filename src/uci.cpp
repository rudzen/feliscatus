#include <iostream>

#include "uci.h"
#include "util.h"
#include "game.h"
#include "position.h"

namespace {

bool parse_option_name(int &param, const char *params[], const int num_params, const char **option_name) {
  while (param < num_params)
  {
    if (util::strieq(params[param++], "name"))
      break;
  }

  if (param < num_params)
  {
    *option_name = params[param++];
    return *option_name != nullptr;
  }
  return false;
}

bool parse_option_value(int &param, const char *params[], const int num_params, const char **option_value) {
  *option_value = nullptr;

  while (param < num_params)
    if (util::strieq(params[param++], "value"))
      break;

  if (param < num_params)
  {
    *option_value = params[param++];
    return *option_value != nullptr;
  }
  return true;
}

}

UCIProtocol::UCIProtocol(ProtocolListener *cb, Game *g)
  : Protocol(cb, g) {
}

void UCIProtocol::post_moves(const Move bestmove, const Move pondermove) {
  fmt::memory_buffer buffer;

  fmt::format_to(buffer, "bestmove {}", display_uci(bestmove));

  if (pondermove)
    fmt::format_to(buffer, " ponder {}", display_uci(pondermove));

  fmt::print("{}\n", fmt::to_string(buffer));
}

void UCIProtocol::post_info(const int d, const int selective_depth, const uint64_t node_count, const uint64_t nodes_per_sec, const TimeUnit time, const int hash_full) {
  fmt::print("info depth {} seldepth {} hashfull {} nodes {} nps {} time {}\n", d, selective_depth, hash_full, node_count, nodes_per_sec, time);
}

void UCIProtocol::post_curr_move(const Move curr_move, const int curr_move_number) {
  fmt::print("info currmove {} currmovenumber {}\n", display_uci(curr_move), curr_move_number);
}

void UCIProtocol::post_pv(const int d, const int max_ply, const uint64_t node_count, const uint64_t nodes_per_second, const TimeUnit time, const int hash_full, const int score, fmt::memory_buffer &pv, const NodeType node_type) {

  fmt::memory_buffer buffer;
  fmt::format_to(buffer, "info depth {} seldepth {} score cp {} ", d, max_ply, score);

  if (node_type == ALPHA)
    fmt::format_to(buffer, "upperbound ");
  else if (node_type == BETA)
    fmt::format_to(buffer, "lowerbound ");

  fmt::print("{}hashfull {} nodes {} nps {} time {} pv {}\n", fmt::to_string(buffer), hash_full, node_count, nodes_per_second, time, fmt::to_string(pv));
}

int UCIProtocol::handle_input(const char *params[], const int num_params) {
  if (num_params < 1)
    return -1;

  if (util::strieq(params[0], "uci"))
  {
    fmt::print("id name Feliscatus 0.1\n");
    fmt::print("id author Gunnar Harms, FireFather, Rudy Alex Kohn\n");
    fmt::print("option name Hash type spin default 1024 min 8 max 65536\n");
    fmt::print("option name Ponder type check default true\n");
    fmt::print("option name Threads type spin default 1 min 1 max 64\n");
    fmt::print("option name UCI_Chess960 type check default false\n");
    fmt::print("uciok\n");
  } else if (util::strieq(params[0], "isready"))
  {
    fmt::print("readyok\n");
  } else if (util::strieq(params[0], "ucinewgame"))
  {
    callback->new_game();
    fmt::print("readyok\n");
  } else if (util::strieq(params[0], "setoption"))
    handle_set_option(params, num_params);
  else if (util::strieq(params[0], "position"))
    handle_position(params, num_params);
  else if (util::strieq(params[0], "go"))
    handle_go(params, num_params, callback);
  else if (util::strieq(params[0], "quit"))
    return 1;
  return 0;
}

int UCIProtocol::handle_go(const char *params[], const int num_params, ProtocolListener *cb) {

  limits.clear();

  for (auto param = 1; param < num_params; param++)
  {
    if (util::strieq(params[param], "movetime"))
    {
      limits.fixed_movetime = true;

      if (++param < num_params)
        limits.movetime = strtol(params[param], nullptr, 10);
    } else if (util::strieq(params[param], "depth"))
    {
      limits.fixed_depth = true;

      if (++param < num_params)
        limits.depth = strtol(params[param], nullptr, 10);
    } else if (util::strieq(params[param], "wtime"))
    {
      if (++param < num_params)
        limits.time[WHITE] = strtol(params[param], nullptr, 10);
    } else if (util::strieq(params[param], "movestogo"))
    {
      if (++param < num_params)
        limits.movestogo = strtol(params[param], nullptr, 10);
    } else if (util::strieq(params[param], "btime"))
    {
      if (++param < num_params)
        limits.time[BLACK] = strtol(params[param], nullptr, 10);
    } else if (util::strieq(params[param], "winc"))
    {
      if (++param < num_params)
        limits.inc[WHITE] = strtol(params[param], nullptr, 10);
    } else if (util::strieq(params[param], "binc"))
    {
      if (++param < num_params)
        limits.inc[BLACK] = strtol(params[param], nullptr, 10);
    } else if (util::strieq(params[param], "infinite"))
      limits.infinite = true;
    else if (util::strieq(params[param], "ponder"))
      limits.ponder = true;
  }
  cb->go(limits);
  return 0;
}

int UCIProtocol::handle_position(const char *params[], const int num_params) const {
  auto param = 1;

  if (num_params < param + 1)
    return -1;

  char fen[128];

  if (util::strieq(params[param], "startpos"))
  {
    strcpy(fen, Game::kStartPosition.data());
    param++;
  } else if (util::strieq(params[param], "fen"))
  {
    if (!util::FENfromParams(params, num_params, param, fen))
      return -1;
    param++;
  } else
    return -1;

  callback->set_fen(fen);

  if (num_params > param && util::strieq(params[param++], "moves"))
  {
    while (param < num_params)
    {
      const auto *const move = game->pos->string_to_move(params[param++]);

      if (move == nullptr)
        return -1;

      game->make_move(*move, true, true);
    }
  }
  return 0;
}

bool UCIProtocol::handle_set_option(const char *params[], const int num_params) const {
  auto param = 1;
  const char *option_name;
  const char *option_value;

  if (parse_option_name(param, params, num_params, &option_name) && parse_option_value(param, params, num_params, &option_value))
    return callback->set_option(option_name, option_value) == 0;

  return false;
}
