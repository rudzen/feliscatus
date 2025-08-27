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

  zobristSide    = rng();
  zobristNopawn  = rng();

  for (auto &z : zobristPst)
    for (auto &zPst : z)
      zPst = rng();

  for (auto &z : zobristCastling)
    z = rng();

  for (auto &z : zobristEpFile)
    z = rng();
}

[[nodiscard]] constexpr Key pst(const Piece pc, const Square sq) const {
    return zobristPst[pc][sq];
}

[[nodiscard]] constexpr Key castle(const int castleRights) const {
    return zobristCastling[castleRights];
}

[[nodiscard]] constexpr Key ep(const File f) const {
    return zobristEpFile[f];
}

[[nodiscard]] constexpr Key side() const {
    return zobristSide;
}

[[nodiscard]] constexpr Key noPawn() const {
    return zobristNopawn;
}

[[nodiscard]] constexpr Key zero() const {
  return 0;
}

private:

[[nodiscard]] constexpr Key seed() const {
  return 1070372;
}

std::array<std::array<Key, PIECE_NB>, SQ_NB> zobristPst{};
std::array<Key, CASTLING_RIGHT_NB> zobristCastling{};
std::array<Key, FILE_NB> zobristEpFile{};
Key zobristSide{};
Key zobristNopawn{};

};

constinit inline const Zobrist zobrist;