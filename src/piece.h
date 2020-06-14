#pragma once

#include <string_view>
#include <cstdio>

// enum PieceType {
//   Pawn, Knight, Bishop, Rook, Queen, King, NoPiece
// };

constexpr int Pawn    = 0;
constexpr int Knight  = 1;
constexpr int Bishop  = 2;
constexpr int Rook    = 3;
constexpr int Queen   = 4;
constexpr int King    = 5;
constexpr int NoPiece = 6;

constexpr std::array<int, 6> piece_values{100, 400, 400, 600, 1200, 0};

constexpr int piece_value(const int p) {
  return piece_values[p & 7];
}

constexpr std::string_view piece_notation = " nbrqk";

inline const char *pieceToString(int piece, char *buf) {
  sprintf(buf, "%c", piece_notation[piece]);
  return buf;
}
