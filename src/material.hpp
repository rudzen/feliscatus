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

#include <array>

#include "types.hpp"

struct Board;
enum Move : uint32_t;

struct Material {
  void clear();

  void remove(Piece pc);

  void add(Piece pc);

  [[nodiscard]]
  int count(Color c, PieceType pt);

  void make_move(Move m);

  [[nodiscard]]
  bool is_kx(Color c);

  [[nodiscard]]
  int value();

  [[nodiscard]]
  int value(Color c);

  [[nodiscard]]
  int pawn_value();

  [[nodiscard]]
  int pawn_count();

  template<Color Us>
  [[nodiscard]]
  int evaluate(int &flags, int eval, const Board *b);

  [[nodiscard]]
  static constexpr int recognize_draw();

  std::array<int, COL_NB> material_value{};

  static constexpr int max_value_without_pawns = 2 * (2 * piece_values[KNIGHT] + 2 * piece_values[BISHOP] + 2 * piece_values[ROOK] + piece_values[QUEEN]);
  static constexpr int max_value               = max_value_without_pawns + 2 * 8 * piece_values[PAWN];

private:

  enum KeyUpdateType {
    Add, Remove
  };

  using Keys = std::array<uint32_t, COL_NB>;

  template<KeyUpdateType Type>
  void update_key(Color c, PieceType pt);

  [[nodiscard]]
  int pawn_count(Color c);

  [[nodiscard]]
  bool is_kx();

  [[nodiscard]]
  int balance();

  [[nodiscard]]
  int KQBKX(int eval, uint32_t key2);

  [[nodiscard]]
  int KQNKX(int eval, uint32_t key2);

  [[nodiscard]]
  int KRBKX(int eval, uint32_t key2);

  [[nodiscard]]
  int KRNKX(int eval, uint32_t key2);

  [[nodiscard]]
  int KRKX(int eval, uint32_t key2);

  [[nodiscard]]
  int KBBKX(int eval, uint32_t key2);

  [[nodiscard]]
  int KBNKX(int eval, uint32_t key2, int pc1, int pc2, Color c1);

  [[nodiscard]]
  int KBNK(int eval, Color c1) const;

  [[nodiscard]]
  int KBKX(int eval, uint32_t key1, uint32_t key2, int pc1, int pc2, Color c1, Color c2, Color c);

  [[nodiscard]]
  int KNKX(int eval, uint32_t key2, int pc1, int pc2, Color c1, Color c2, Color c);

  [[nodiscard]]
  int KNNKX(int eval, uint32_t key2, int pc1);

  [[nodiscard]]
  int KKx(int eval, int pc1, int pc2, Color c1);

  [[nodiscard]]
  int KBxKX(int eval, uint32_t key1, uint32_t key2, Color c1);

  [[nodiscard]]
  int KBxKx(int eval, uint32_t key1, uint32_t key2, Color c1);

  // fen 8/6k1/8/8/3K4/5B1P/8/8 w - - 0 1
  [[nodiscard]]
  int KBpK(int eval, Color c1);

  [[nodiscard]]
  int KxKx(int eval, int pc1, int pc2, Color c1);

  [[nodiscard]]
  int KpK(int eval, Color c1);

  [[nodiscard]]
  int draw_score();

  int drawish{};
  int material_flags{};
  Keys key{};
  const Board *board{};

  static constexpr int RECOGNIZEDDRAW = 1;
};

inline bool Material::is_kx(const Color c) {
  return key[c] == (key[c] & 15);
}

inline bool Material::is_kx() {
  return is_kx(WHITE) && is_kx(BLACK);
}

inline int Material::value() {
  return material_value[WHITE] + material_value[BLACK];
}

inline int Material::value(const Color c) {
  return material_value[c];
}

inline int Material::pawn_count() {
  return static_cast<int>(key[WHITE] & 15) + static_cast<int>(key[BLACK] & 15);
}

inline int Material::balance() {
  return material_value[WHITE] - material_value[BLACK];
}

inline int Material::pawn_count(const Color c) {
  return static_cast<int>(key[c] & 15);
}

constexpr int Material::recognize_draw() {
   return RECOGNIZEDDRAW;
}
