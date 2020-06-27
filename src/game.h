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

#include <cstdint>
#include <string_view>

#include "types.h"
#include "board.h"
#include "position.h"

enum Move : uint32_t;

class Game final {
public:
  Game();
  explicit Game(std::string_view fen);

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

  std::array<Position, 512> position_list{};
  Position *pos;
  Board board{};
  bool chess960;
  bool xfen;

  std::array<int, sq_nb> castle_rights_mask{};

  static constexpr std::string_view kStartPosition = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

private:
  void update_position(Position *p) const;
};
