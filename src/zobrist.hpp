/*
  Feliscatus, a UCI chess playing engine derived from Tomcat 1.0 (Bobcat 8.0)
  Copyright (C) 2008-2016 Gunnar Harms (Bobcat author)
  Copyright (C) 2017      FireFather (Tomcat author)
  Copyright (C) 2020-2022 Rudy Alex Kohn

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

constexpr explicit Zobrist() {
  PRNG rng(seed());

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

constexpr Key pst(const Piece pc, const Square sq) const {
    return zobrist_pst[pc][sq];
}

constexpr Key castle(const int castle_rights) const {
    return zobrist_castling[castle_rights];
}

constexpr Key ep(const File f) const {
    return zobrist_ep_file[f];
}

constexpr Key side() const {
    return zobrist_side;
}

constexpr Key no_pawn() const {
    return zobrist_nopawn;
}

constexpr Key zero() const {
  return 0;
}

private:

constexpr Key seed() const {
  return 1070372;
}

std::array<std::array<Key, PIECE_NB>, SQ_NB> zobrist_pst{};
std::array<Key, CASTLING_RIGHT_NB> zobrist_castling{};
std::array<Key, FILE_NB> zobrist_ep_file{};
Key zobrist_side{};
Key zobrist_nopawn{};

};

constinit inline const Zobrist zobrist;