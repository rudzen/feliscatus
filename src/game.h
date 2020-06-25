#pragma once

#include <cstdint>
#include <string_view>
#include "types.h"
#include "board.h"
#include "position.h"

enum Move : uint32_t;

class Game final {
public:
  Game();

  bool make_move(Move m, bool check_legal, bool calculate_in_check);

  void unmake_move();

  bool make_null_move();

  uint64_t calculate_key();

  [[nodiscard]]
  bool is_repetition() const;

  [[nodiscard]]
  int half_move_count() const;

  int new_game(std::string_view fen);

  int set_fen(std::string_view fen);

  [[nodiscard]]
  std::string get_fen() const;

  [[nodiscard]]
  bool setup_castling(std::string_view s);

  void copy(Game *other);

  [[nodiscard]]
  std::string move_to_string(Move m) const;

  void print_moves() const;

  std::array<Position, 256> position_list{};
  Position *pos;
  Board board{};
  bool chess960;
  bool xfen;

  std::array<int, sq_nb> castle_rights_mask{};

  static constexpr std::string_view kStartPosition = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

private:
  void update_position(Position *p) const;
};
