#pragma once

#include <string_view>

constexpr int Pawn    = 0;
constexpr int Knight  = 1;
constexpr int Bishop  = 2;
constexpr int Rook    = 3;
constexpr int Queen   = 4;
constexpr int King    = 5;
constexpr int NoPiece = 6;

constexpr std::string_view piece_notation = " nbrqk";

inline const char *pieceToString(int piece, char *buf) {
  sprintf(buf, "%c", piece_notation[piece]);
  return buf;
}
