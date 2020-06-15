#pragma once

#include <cstdint>
#include "square.h"
#include "types.h"

constexpr int QUIET      = 1;
constexpr int DOUBLEPUSH = 2;
constexpr int CASTLE     = 4;
constexpr int EPCAPTURE  = 8;
constexpr int PROMOTION  = 16;
constexpr int CAPTURE    = 32;

constexpr int move_piece(const uint32_t move) {
  return move >> 26 & 15;
}

constexpr void move_set_piece(uint32_t &move, const int piece) {
  move |= piece << 26;
}

constexpr int move_captured(const uint32_t move) {
  return move >> 22 & 15;
}

constexpr void move_set_captured(uint32_t &move, const int piece) {
  move |= piece << 22;
}

constexpr int move_promoted(const uint32_t move) {
  return move >> 18 & 15;
}

constexpr void move_set_promoted(uint32_t &move, const int piece) {
  move |= piece << 18;
}

constexpr int move_type(const uint32_t move) {
  return move >> 12 & 63;
}

constexpr void move_set_type(uint32_t &move, const int type) {
  move |= type << 12;
}

constexpr Square move_from(const uint32_t move) {
  return static_cast<Square>(move >> 6 & 63);
}

constexpr void move_set_from(uint32_t &move, const Square sq) {
  move |= sq << 6;
}

constexpr Square move_to(const uint32_t move) {
  return static_cast<Square>(move & 63);
}

constexpr void move_set_to(uint32_t &move, const Square sq) {
  move |= sq;
}

constexpr int move_piece_type(const uint32_t move) {
  return move >> 26 & 7;
}

constexpr Color move_side(const uint32_t m) {
  return static_cast<Color>(m >> 29 & 1);
}

constexpr int side_mask(const uint32_t m) {
  return move_side(m) << 3;
}

constexpr int is_capture(const uint32_t m) {
  return move_type(m) & (CAPTURE | EPCAPTURE);
}

constexpr int is_ep_capture(const uint32_t m) {
  return move_type(m) & EPCAPTURE;
}

constexpr int is_castle_move(const uint32_t m) {
  return move_type(m) & CASTLE;
}

constexpr int is_promotion(const uint32_t m) {
  return move_type(m) & PROMOTION;
}

constexpr bool is_queen_promotion(const uint32_t m) {
  return is_promotion(m) && (move_promoted(m) & 7) == Queen;
}

constexpr bool is_null_move(const uint32_t m) {
  return m == 0;
}

constexpr void init_move(uint32_t &move, const int piece, const int captured, const Square from, const Square to, const int type, const int promoted) {
  move = 0;
  move_set_piece(move, piece);
  move_set_captured(move, captured);
  move_set_promoted(move, promoted);
  move_set_from(move, from);
  move_set_to(move, to);
  move_set_type(move, type);
}