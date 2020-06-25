#include <iostream>
#include <string>
#include "uci.h"
#include "util.h"
#include "game.h"
#include "position.h"

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

int UCIProtocol::handle_go(std::istringstream& input) {

  limits.clear();

  std::string token;

  while (input >> token)
    if (token == "wtime")
      input >> limits.time[WHITE];
    else if (token == "btime")
      input >> limits.time[BLACK];
    else if (token == "winc")
      input >> limits.inc[WHITE];
    else if (token == "binc")
      input >> limits.inc[BLACK];
    else if (token == "movestogo")
      input >> limits.movestogo;
    else if (token == "depth")
      input >> limits.depth;
    else if (token == "movetime")
      input >> limits.movetime;
    else if (token == "infinite")
      limits.infinite = 1;
    else if (token == "ponder")
      limits.ponder = true;

  return 0;
}

void UCIProtocol::handle_position(Game *g, std::istringstream &input) const {

  std::string token, fen;

  input >> token;

  if (token == "startpos")
  {
    fen = Game::kStartPosition;

    // get rid of "moves" token
    input >> token;
  } else if (token == "fen")
    while (input >> token && token != "moves")
      fen += token + ' ';
  else
    return;

  g->set_fen(fen);

  auto m{MOVE_NONE};

  // parse any moves if they exist
  while (input >> token && (m = *g->pos->string_to_move(token)) != MOVE_NONE)
    g->make_move(m, false, true);
}

bool UCIProtocol::handle_set_option(std::istringstream& input) const {

    std::string token, option_name, option_value;

    // get rid of "name"
    input >> token;

    // read possibly spaced name
    while (input >> token && token != "value")
      option_name += (option_name.empty() ? "" : " ") + token;

    // read possibly spaced value
    while (input >> token)
      option_value += (option_value.empty() ? "" : " ") + token;

    auto valid_option = !option_name.empty() || !option_value.empty();

    if (valid_option)
      valid_option = callback->set_option(option_name, option_value);

    return valid_option;
}
