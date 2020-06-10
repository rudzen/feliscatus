#pragma once

#include <cstdint>

constexpr int movePiece(const uint32_t move) {
  return (move >> 26) & 15;
}

constexpr void moveSetPiece(uint32_t &move, const int piece) {
  move |= (piece << 26);
}

constexpr int moveCaptured(const uint32_t move) {
  return (move >> 22) & 15;
}

constexpr void moveSetCaptured(uint32_t &move, const int piece) {
  move |= (piece << 22);
}

constexpr int movePromoted(const uint32_t move) {
  return (move >> 18) & 15;
}

constexpr void moveSetPromoted(uint32_t &move, const int piece) {
  move |= (piece << 18);
}

constexpr int moveType(const uint32_t move) {
  return (move >> 12) & 63;
}

constexpr void moveSetType(uint32_t &move, const int type) {
  move |= (type << 12);
}

constexpr uint32_t moveFrom(const uint32_t move) {
  return (move >> 6) & 63;
}

constexpr void moveSetFrom(uint32_t &move, const int sq) {
  move |= (sq << 6);
}

constexpr uint32_t moveTo(const uint32_t move) {
  return move & 63;
}

constexpr void moveSetTo(uint32_t &move, const int sq) {
  move |= sq;
}

constexpr int movePieceType(const uint32_t move) {
  return (move >> 26) & 7;
}

constexpr int moveSide(const uint32_t m) {
  return (m >> 29) & 1;
}

constexpr int sideMask(const uint32_t m) {
  return moveSide(m) << 3;
}

const int QUIET      = 1;
const int DOUBLEPUSH = 2;
const int CASTLE     = 4;
const int EPCAPTURE  = 8;
const int PROMOTION  = 16;
const int CAPTURE    = 32;

constexpr int isCapture(const uint32_t m) {
  return moveType(m) & (CAPTURE | EPCAPTURE);
}

constexpr int isEpCapture(const uint32_t m) {
  return moveType(m) & EPCAPTURE;
}

constexpr int isCastleMove(const uint32_t m) {
  return moveType(m) & CASTLE;
}

constexpr int isPromotion(const uint32_t m) {
  return moveType(m) & PROMOTION;
}

constexpr bool isQueenPromotion(const uint32_t m) {
  return isPromotion(m) && (movePromoted(m) & 7) == Queen;
}

constexpr bool isNullMove(const uint32_t m) {
  return m == 0;
}

constexpr void initMove(uint32_t &move, const int piece, const int captured, const uint64_t from, const uint64_t to, const int type, const int promoted) {
  move = 0;
  moveSetPiece(move, piece);
  moveSetCaptured(move, captured);
  moveSetPromoted(move, promoted);
  moveSetFrom(move, from);
  moveSetTo(move, to);
  moveSetType(move, type);
}