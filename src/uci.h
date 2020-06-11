
#pragma once

// TODO : Replace with portable code
#include <conio.h>

#include <iostream>
#include "game.h"

constexpr int FIXED_MOVE_TIME    = 1;
constexpr int FIXED_DEPTH        = 2;
constexpr int INFINITE_MOVE_TIME = 4;
constexpr int PONDER_SEARCH      = 8;

class ProtocolListener {
public:
  virtual int new_game()                                                                = 0;
  virtual int set_fen(const char *fen)                                                  = 0;
  virtual int go(int wtime, int btime, int movestogo, int winc, int binc, int movetime) = 0;
  virtual void ponder_hit()                                                             = 0;
  virtual void stop()                                                                   = 0;
  virtual int set_option(const char *name, const char *value)                           = 0;
};

class Protocol {
public:
  Protocol(ProtocolListener *cb, Game *g) : callback(cb), game(g) { }

  virtual ~Protocol() = default;

  virtual int handle_input(const char *params[], int num_params)        = 0;
  virtual void check_input()                                            = 0;
  virtual void post_moves(const char *bestmove, const char *pondermove) = 0;

  virtual void post_info(int depth, int selective_depth, uint64_t node_count, uint64_t nodes_per_sec, uint64_t time, int hash_full) = 0;

  virtual void post_curr_move(uint32_t curr_move, int curr_move_number) = 0;

  virtual void post_pv(int depth, int max_ply, uint64_t node_count, uint64_t nodes_per_second, uint64_t time, int hash_full, int score, const char *pv, int node_type) = 0;

  [[nodiscard]]
  int is_analysing() const noexcept { return flags & (INFINITE_MOVE_TIME | PONDER_SEARCH); }

  [[nodiscard]]
  int is_fixed_time() const noexcept { return flags & FIXED_MOVE_TIME; }

  [[nodiscard]]
  int is_fixed_depth() const noexcept { return flags & FIXED_DEPTH; }

  [[nodiscard]]
  int get_depth() const noexcept { return depth; }

  void set_flags(const int flags) noexcept { this->flags = flags; }

protected:
  int flags{};
  int depth{};
  ProtocolListener *callback;
  Game *game;
};

class UCIProtocol final : public Protocol {
public:
  UCIProtocol(ProtocolListener *callback, Game *game) : Protocol(callback, game) {}

  static int input_available() {
    //	if (stdin->cnt > 0) return 1;

    static auto *h_input = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode;
    static auto console = GetConsoleMode(h_input, &mode);

    if (!console)
    {
      DWORD total_bytes_avail;
      if (!PeekNamedPipe(h_input, nullptr, 0, nullptr, &total_bytes_avail, nullptr))
        return true;
      return total_bytes_avail;
    }
    return _kbhit();
  }

  void check_input() override {
    while (input_available())
    {
      char str[128];
      std::cin.getline(str, 128);

      if (!*str)
        return;

      if (strieq(trim(str), "stop"))
      {
        flags &= ~(INFINITE_MOVE_TIME | PONDER_SEARCH);
        callback->stop();
      } else if (strieq(trim(str), "ponderhit"))
      {
        flags &= ~(INFINITE_MOVE_TIME | PONDER_SEARCH);
        callback->ponder_hit();
      }
    }
  }

  void post_moves(const char *bestmove, const char *pondermove) override {
    while (flags & (INFINITE_MOVE_TIME | PONDER_SEARCH))
    {
      Sleep(10);
      check_input();
    }
    printf("bestmove %s", bestmove);

    if (pondermove)
      printf(" ponder %s", pondermove);

    printf("\n");
  }

  void post_info(const int depth, const int selective_depth, const uint64_t node_count, const uint64_t nodes_per_sec, const uint64_t time, const int hash_full) override {
    printf("info depth %d seldepth %d hashfull %d nodes %llu nps %llu time %llu\n", depth, selective_depth, hash_full, node_count, nodes_per_sec, time);
  }

  void post_curr_move(const uint32_t curr_move, const int curr_move_number) override {
    char move_buf[32];

    printf("info currmove %s currmovenumber %d\n", game->move_to_string(curr_move, move_buf), curr_move_number);
  }

  void post_pv(const int depth, const int max_ply, const uint64_t node_count, const uint64_t nodes_per_second, const uint64_t time, const int hash_full, const int score, const char *pv, const int node_type) override {
    char bound[24];

    if (node_type == 4)
      strcpy(bound, "upperbound ");
    else if (node_type == 2)
      strcpy(bound, "lowerbound ");
    else
      bound[0] = 0;

    printf("info depth %d seldepth %d score cp %d %s hashfull %d nodes %llu nps %llu time %llu pv %s\n", depth, max_ply, score, bound, hash_full, node_count, nodes_per_second, time, pv);
  }

  int handle_input(const char *params[], const int num_params) override {
    if (num_params < 1)
      return -1;

    if (strieq(params[0], "uci"))
    {
      printf("id name Feliscatus 0.1\n");
      printf("id author Gunnar Harms\n");
      printf("option name Hash type spin default 1024 min 8 max 65536\n");
      printf("option name Ponder type check default true\n");
      printf("option name Threads type spin default 1 min 1 max 64\n");
      printf("option name UCI_Chess960 type check default false\n");
      printf("uciok\n");
    } else if (strieq(params[0], "isready"))
    {
      printf("readyok\n");
    } else if (strieq(params[0], "ucinewgame"))
    {
      callback->new_game();
      printf("readyok\n");
    } else if (strieq(params[0], "setoption"))
    {
      handle_set_option(params, num_params);
    } else if (strieq(params[0], "position"))
    {
      handle_position(params, num_params);
    } else if (strieq(params[0], "go"))
    {
      handle_go(params, num_params, callback);
    } else if (strieq(params[0], "quit"))
    { return 1; }
    return 0;
  }

  int handle_go(const char *params[], const int num_params, ProtocolListener *callback) {
    auto wtime     = 0;
    auto btime     = 0;
    auto winc      = 0;
    auto binc      = 0;
    auto movetime  = 0;
    auto movestogo = 0;

    flags = 0;

    for (auto param = 1; param < num_params; param++)
    {
      if (strieq(params[param], "movetime"))
      {
        flags |= FIXED_MOVE_TIME;

        if (++param < num_params)
          movetime = strtol(params[param], nullptr, 10);
      } else if (strieq(params[param], "depth"))
      {
        flags |= FIXED_DEPTH;

        if (++param < num_params)
          depth = strtol(params[param], nullptr, 10);
      } else if (strieq(params[param], "wtime"))
      {
        if (++param < num_params)
          wtime = strtol(params[param], nullptr, 10);
      } else if (strieq(params[param], "movestogo"))
      {
        if (++param < num_params)
          movestogo = strtol(params[param], nullptr, 10);
      } else if (strieq(params[param], "btime"))
      {
        if (++param < num_params)
          btime = strtol(params[param], nullptr, 10);
      } else if (strieq(params[param], "winc"))
      {
        if (++param < num_params)
          winc = strtol(params[param], nullptr, 10);
      } else if (strieq(params[param], "binc"))
      {
        if (++param < num_params)
          binc = strtol(params[param], nullptr, 10);
      } else if (strieq(params[param], "infinite"))
        flags |= INFINITE_MOVE_TIME;
      else if (strieq(params[param], "ponder"))
        flags |= PONDER_SEARCH;
    }
    callback->go(wtime, btime, movestogo, winc, binc, movetime);
    return 0;
  }

  int handle_position(const char *params[], const int num_params) const {
    auto param = 1;

    if (num_params < param + 1)
      return -1;

    char fen[128];

    if (strieq(params[param], "startpos"))
    {
      strcpy(fen, Game::kStartPosition);
      param++;
    } else if (strieq(params[param], "fen"))
    {
      if (!FENfromParams(params, num_params, param, fen))
        return -1;
      param++;
    } else
    { return -1; }
    callback->set_fen(fen);

    if ((num_params > param) && (strieq(params[param++], "moves")))
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

  bool handle_set_option(const char *params[], const int num_params) const {
    auto param = 1;
    const char *option_name;
    const char *option_value;

    if (parse_option_name(param, params, num_params, &option_name) && parse_option_value(param, params, num_params, &option_value))
    {
      return callback->set_option(option_name, option_value) == 0;
    }
    return false;
  }

  static bool parse_option_name(int &param, const char *params[], int num_params, const char **option_name) {
    while (param < num_params)
    {
      if (strieq(params[param++], "name"))
        break;
    }

    if (param < num_params)
    {
      *option_name = params[param++];
      return *option_name != nullptr;
    }
    return false;
  }

  static bool parse_option_value(int &param, const char *params[], const int num_params, const char **option_value) {
    *option_value = nullptr;

    while (param < num_params)
      if (strieq(params[param++], "value"))
        break;

    if (param < num_params)
    {
      *option_value = params[param++];
      return *option_value != nullptr;
    }
    return true;
  }
};