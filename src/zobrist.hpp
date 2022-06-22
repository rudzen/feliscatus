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

#include "types.hpp"
#include "prng.hpp"

struct Zobrist final {

static constexpr Key seed = 1070372;
static constexpr Key zero = 0;

constexpr Zobrist() {
  PRNG<Key> rng(seed);

  zobrist_side   = rng();
  zobrist_nopawn = rng();

  for (auto &z : zobrist_pst)
    for (auto &z_pst : z)
      z_pst = rng();

  for (auto &z : zobrist_castling)
    z = rng();

  for (auto &z : zobrist_ep_file)
    z = rng();
}

constexpr Key pst(const Piece pc, const Square sq) {
    return zobrist_pst[pc][sq];
}

constexpr Key castle(const int castle_rights) {
    return zobrist_castling[castle_rights];
}

constexpr Key ep(const File f) {
    return zobrist_ep_file[f];
}

constexpr Key side() {
    return zobrist_side;
}

constexpr Key no_pawn() {
    return zobrist_nopawn;
}

private:

Key zobrist_pst[PIECE_NB][SQ_NB]{};
std::array<Key, CASTLING_RIGHT_NB> zobrist_castling{};
std::array<Key, FILE_NB> zobrist_ep_file{};
Key zobrist_side, zobrist_nopawn{};

};

constinit inline Zobrist zobrist;