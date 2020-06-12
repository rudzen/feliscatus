#pragma once

#include <algorithm>
#include <memory>
#include "zobrist.h"
#include "uci.h"
#include "game.h"
#include "eval.h"
#include "see.h"
#include "search.h"
#include "hash.h"
#include "worker.h"
#include "perft.h"
#include "tune.h"

class Felis : public ProtocolListener {
public:
  Felis() : num_threads(1) {}

  virtual ~Felis() = default;

  int new_game() override {
    game->new_game(Game::kStartPosition);
    // TODO : test effect of not clearing
    //pawnt->clear();
    //TT.clear();
    return 0;
  }

  int set_fen(const char *fen) override { return game->new_game(fen); }

  int go(const int wtime = 0, const int btime = 0, const int movestogo = 0, const int winc = 0, const int binc = 0, const int movetime = 5000) override {
    game->pos->pv_length = 0;

    if (game->pos->pv_length == 0)
      go_search(wtime, btime, movestogo, winc, binc, movetime);

    if (game->pos->pv_length)
    {
      char best_move[12];
      char ponder_move[12];

      protocol->post_moves(game->move_to_string(search->pv[0][0].move, best_move), game->pos->pv_length > 1 ? game->move_to_string(search->pv[0][1].move, ponder_move) : nullptr);

      game->make_move(search->pv[0][0].move, true, true);
    }
    return 0;
  }

  void ponder_hit() override { search->search_time += search->start_time.millisElapsed(); }

  void stop() override { search->stop_search = true; }

  virtual bool make_move(const char *m) {
    const uint32_t *move = game->pos->string_to_move(m);
    return move ? game->make_move(*move, true, true) : false;
  }

  void go_search(const int wtime, const int btime, const int movestogo, const int winc, const int binc, const int movetime) {
    // Shared transposition table
    start_workers();
    search->go(wtime, btime, movestogo, winc, binc, movetime, num_threads);
    stop_workers();
  }

  void start_workers() {
    for (auto &worker : workers)
      worker.start(game.get(), pawnt.get());
    //for (int i = 0; i < num_threads - 1; i++)
    //  workers[i].start(game, transt, pawnt);
  }

  void stop_workers() const {
    for (const auto &worker : workers)
      worker.stop();
    //for (int i = 0; i < num_threads - 1; i++)
    //  workers[i].stop();
  }

  int set_option(const char *name, const char *value) override {
    char buf[1024];
    strcpy(buf, "");

    if (value != nullptr)
    {
      if (strieq("Hash", name))
      {
        TT.init(std::clamp(static_cast<int>(strtol(value, nullptr, 10)), 8, 65536));
        _snprintf(buf, sizeof(buf), "Hash:%d", TT.get_size_mb());
      } else if (strieq("Threads", name) || strieq("NumThreads", name))
      {
        num_threads = std::clamp(static_cast<int>(strtol(value, nullptr, 10)), 1, 64);
        _snprintf(buf, sizeof(buf), "Threads:%d", num_threads);
        workers.resize(num_threads - 1);
        workers.shrink_to_fit();
      } else if (strieq("UCI_Chess960", name))
      {
        game->chess960 = strieq(value, "true");
        _snprintf(buf, sizeof(buf), "UCI_Chess960 ", game->chess960 ? on.data() : off.data());
      } else if (strieq("UCI_Chess960_Arena", name))
      {
        game->chess960 = game->xfen = strieq(value, "true");
      }
    }
    return 0;
  }

  static void init() {
    bitboard::init();
    attacks::init();
    zobrist::init();
    squares::init();
  }

  int run() {
    setbuf(stdout, nullptr);

    bitboard::init();
    attacks::init();
    zobrist::init();
    squares::init();

    game     = std::make_unique<Game>();
    protocol = std::make_unique<UCIProtocol>(this, game.get());
    pawnt    = std::make_unique<PawnHashTable>(8);
    see      = std::make_unique<See>(game.get());
    eval     = std::make_unique<Eval>(*game, pawnt.get());
    search   = std::make_unique<Search>(protocol.get(), game.get(), eval.get(), see.get());

    new_game();

    auto console_mode = true;
    auto quit         = 0;
    char line[16384];

    while (quit == 0)
    {
      game->pos->generate_moves();

      (void)fgets(line, 16384, stdin);

      if (feof(stdin))
        exit(0);

      char *tokens[1024];
      const int num_tokens = tokenize(trim(line), tokens, 1024);

      if (num_tokens == 0)
        continue;

      if (strieq(tokens[0], "uci") || !console_mode)
      {
        quit         = protocol->handle_input(const_cast<const char **>(tokens), num_tokens);
        console_mode = false;
      } else if (strieq(tokens[0], "go"))
      {
        protocol->set_flags(INFINITE_MOVE_TIME);
        go();
      } else if (strieq(tokens[0], "perft"))
      {
        Perft(game.get()).perft(6);
      } else if (strieq(tokens[0], "divide"))
      {
        Perft(game.get()).perft_divide(6);
      } else if (strieq(tokens[0], "tune"))
      {
        Stopwatch sw;
        eval::Tune(*game, *see, *eval);
        const double seconds = sw.millisElapsed() / 1000.;
        printf("%f\n", seconds);
      } else if (strieq(tokens[0], "quit"))
      {
        quit = 1;
      } else if (strieq(tokens[0], "exit"))
      {
        quit = 1;
      }

      for (int i = 0; i < num_tokens; i++)
      {
        delete[] tokens[i];
      }
    }
    return 0;
  }

public:
  std::unique_ptr<Game> game;
  std::unique_ptr<Eval> eval;
  std::unique_ptr<See> see;
  std::unique_ptr<Search> search;
  std::unique_ptr<Protocol> protocol;
  std::unique_ptr<PawnHash> pawnt;
  std::vector<Worker> workers{};
  std::size_t num_threads;

  static constexpr std::string_view on = "ON";
  static constexpr std::string_view off = "OFF";
};
