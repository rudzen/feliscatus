#pragma once

#include <cstdint>
#include <string_view>
#include "types.h"
#include "square.h"
#include "board.h"
#include "position.h"

class Game final {
public:
  Game();

  bool make_move(uint32_t m, bool check_legal, bool calculate_in_check);

  void unmake_move();

  bool make_null_move();

  uint64_t calculate_key();

  [[nodiscard]]
  bool is_repetition() const;

  [[nodiscard]]
  int half_move_count() const;

  void add_piece(int p, Color c, Square sq);

  int new_game(std::string_view fen);

  int set_fen(std::string_view fen);

  [[nodiscard]]
  std::string get_fen() const;

  [[nodiscard]]
  int setup_castling(const char **p);

  void copy(Game *other);

  std::string move_to_string(uint32_t m) const;

  void print_moves() const;

public:
  std::array<Position, 2000> position_list{};
  Position *pos;
  Board board{};
  bool chess960;
  bool xfen;

  static constexpr std::string_view kStartPosition = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
};
