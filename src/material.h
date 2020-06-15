#pragma once

#include <array>
#include "types.h"

struct Board;

struct Material {
  void clear();

  void remove(int p);

  void add(int p);

  void update_key(Color c, int p, int delta);

  [[nodiscard]]
  int count(Color c, int p);

  void make_move(uint32_t m);

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

  [[nodiscard]]
  int evaluate(int &flags, int eval, Color side_to_move, const Board *b);

  [[nodiscard]]
  static constexpr int recognize_draw();

  std::array<int, 2> material_value{};

  static constexpr int max_value_without_pawns = 2 * (2 * piece_values[Knight] + 2 * piece_values[Bishop] + 2 * piece_values[Rook] + piece_values[Queen]);
  static constexpr int max_value               = max_value_without_pawns + 2 * 8 * piece_values[Pawn];

private:

  [[nodiscard]]
  int pawn_count(Color side);

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
  int KBNKX(int eval, uint32_t key2, int pc1, int pc2, Color side1);

  [[nodiscard]]
  int KBNK(int eval, Color side1) const;

  [[nodiscard]]
  int KBKX(int eval, uint32_t key1, uint32_t key2, int pc1, int pc2, Color side1, Color side2, Color side_to_move);

  [[nodiscard]]
  int KNKX(int eval, uint32_t key2, int pc1, int pc2, Color side1, Color side2, Color side_to_move);

  [[nodiscard]]
  int KNNKX(int eval, uint32_t key2, int pc1);

  [[nodiscard]]
  int KKx(int eval, uint32_t key1, uint32_t key2, int pc1, int pc2, Color side1);

  [[nodiscard]]
  int KBxKX(int eval, uint32_t key1, uint32_t key2, Color side1);

  [[nodiscard]]
  int KBxKx(int eval, uint32_t key1, uint32_t key2, Color side1);

  // fen 8/6k1/8/8/3K4/5B1P/8/8 w - - 0 1
  [[nodiscard]]
  int KBpK(int eval, Color side1);

  [[nodiscard]]
  int KxKx(int eval, [[maybe_unused]] uint32_t key1, [[maybe_unused]] uint32_t key2, int pc1, int pc2, Color side1);

  [[nodiscard]]
  int KpK(int eval, Color side1);

  [[nodiscard]]
  int draw_score();

  int drawish{};
  int material_flags{};
  std::array<uint32_t, COL_NB> key{};
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

inline int Material::pawn_count(const Color side) {
  return static_cast<int>(key[side] & 15);
}

constexpr int Material::recognize_draw() {
   return RECOGNIZEDDRAW;
}
