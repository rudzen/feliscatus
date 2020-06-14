#pragma once

#include <cstdint>
#include <string_view>
#include "position.h"
#include "types.h"

// TODO : Move the rest of privately used only functions to implementation
class Game {
public:
  Game() : position_list(new Position[2000]), pos(position_list), chess960(false), xfen(false) {
    for (auto i = 0; i < 2000; i++)
      position_list[i].board = &board;
  }

  virtual ~Game() { delete[] position_list; }

  bool make_move(uint32_t m, bool check_legal, bool calculate_in_check);

  void unmake_move();

  bool make_null_move();

  uint64_t calculate_key();

  [[nodiscard]]
  bool is_repetition() const;

  [[nodiscard]]
  int half_move_count() const;

  void add_piece(int p, Color c, Square sq);

  int new_game(const char *fen);

  int set_fen(const char *fen);

  [[nodiscard]]
  char *get_fen() const;

  [[nodiscard]]
  int setup_castling(const char **p);

  void copy(Game *other);

  const char *move_to_string(uint32_t m, char *buf) const;

  void print_moves() const;

public:
  Position *position_list;
  Position *pos;
  Board board;
  bool chess960;
  bool xfen;

  static constexpr std::string_view kStartPosition = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
};
