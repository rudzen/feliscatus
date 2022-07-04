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

#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <numeric>
#include <span>

#include <fmt/format.h>

#include "polyglot.hpp"
#include "polyglot_keys.hpp"
#include "board.hpp"
#include "bitboard.hpp"
#include "moves.hpp"

namespace
{

constexpr std::array<CastlingRight, 4> poly_castles{WHITE_OO, WHITE_OOO, BLACK_OO, BLACK_OOO};

constexpr std::uint64_t get_piece_key(const Piece pc, const Square sq)
{
  return Polyglot::Keys::pc_key(pc, sq);
}

constexpr std::uint64_t get_castle_key(const CastlingRight cr) {
  const auto end = std::find(poly_castles.cbegin(), poly_castles.cend(), cr);
  const auto idx = std::distance(poly_castles.cbegin(), end);
  return Polyglot::Keys::castle_key(idx);
}

constexpr std::uint64_t get_side_key(const Color c) {
  if (c == WHITE)
    return Polyglot::Keys::side_key();

  return 0ULL;
}

constexpr std::uint64_t get_en_passant_key(const File f) {
  return Polyglot::Keys::en_passant_key(f);
}

std::uint64_t hash_pieces(Board *board)
{
  std::uint64_t hash{};
  auto pieces = board->pieces();

  while (pieces)
  {
    const auto sq = pop_lsb(&pieces);
    const auto pc = board->piece(sq);
    hash ^= get_piece_key(pc, sq);
  }

  return hash;
}

std::uint64_t hash_castle(Board *board)
{
  if (!board->can_castle())
    return 0ULL;

  const auto accumulator = [&board](const std::uint64_t r, const CastlingRight cr) {
    return board->can_castle(cr) ? r ^ get_castle_key(cr) : r;
  };

  return std::accumulate(poly_castles.cbegin(), poly_castles.cend(), 0ULL, accumulator);
}

std::uint64_t hash_enpassant(const Square ep_sq)
{
  if (ep_sq != NO_SQ)
    return get_en_passant_key(file_of(ep_sq));

  return 0;
}

std::uint64_t hash_turn(const Color stm)
{
  return get_side_key(stm);
}

std::uint64_t poly_key(Board *board)
{
  return hash_pieces(board) ^ hash_castle(board) ^ hash_turn(board->side_to_move())
       ^ hash_enpassant(board->en_passant_square());
}

///
/// "move" is a bit field with the following meaning
/// (bit 0 is the least significant bit)
/// bits                meaning
/// ===================================
/// 0,1,2               to file
/// 3,4,5               to row
/// 6,7,8               from file
/// 9,10,11             from row
/// 12,13,14            promotion piece
/// "promotion piece" is encoded as follows
/// none       0
/// knight     1
/// bishop     2
/// rook       3
/// queen      4
///
/// If the move is "0" (a1a1) then it should simply be ignored.
///
Move decode(Board *board, std::uint16_t move)
{
  if (!move)
    return MOVE_NONE;

  const auto to_f     = static_cast<File>(move & 0x7);
  const auto to_r     = static_cast<Rank>((move & 0x38) >> 3);
  const auto from_f   = static_cast<File>((move & 0x1C0) >> 6);
  const auto from_r   = static_cast<Rank>((move & 0xE00) >> 9);
  const auto promoted = static_cast<PieceType>((move & 0x7000) >> 12);
  const auto from     = make_square(from_f, from_r);
  const auto to       = make_square(to_f, to_r);
  const auto pc       = board->piece(from);
  const auto pt       = type_of(pc);

  // check castleling move

  if (pt == KING)
  {
    if (from == E1 && to == H1)
      return init_move<CASTLE>(pc, NO_PIECE, from, G1, NO_PIECE);

    if (from == E1 && to == A1)
      return init_move<CASTLE>(pc, NO_PIECE, from, A1, NO_PIECE);

    if (from == E8 && to == H8)
      return init_move<CASTLE>(pc, NO_PIECE, from, G8, NO_PIECE);

    if (from == E8 && to == A8)
      return init_move<CASTLE>(pc, NO_PIECE, from, C8, NO_PIECE);
  }

  auto ml = MoveList<LEGALMOVES>(board);

  const auto from_to_matches = [&from, &to](const Move m) {
    return from == move_from(m) && to == move_to(m);
  };

  const auto m = std::find_if(ml.cbegin(), ml.cend(), from_to_matches);

  if (m != ml.end())
  {
    const auto mt = type_of(*m);
    if (mt & PROMOTION)
      return init_move(pc, move_captured(*m), from, to, mt, make_piece(promoted, board->side_to_move()));
    if (mt & EPCAPTURE)
      return init_move<EPCAPTURE>(pc, make_piece(PAWN, ~color_of(pc)), from, to, NO_PIECE);
    return *m;
  }

  return MOVE_NONE;
}

}   // namespace

///
/// Opens a binary polyglot book and parses entries
///
void PolyBook::open(const std::string_view path)
{
  auto book_file = std::ifstream(path.data(), std::ios::binary | std::ios::ate);

  if (!book_file)
  {
    fmt::print("Unable to open book. path={}\n", path);
    return;
  }

  if (current_book_ == path)
  {
    fmt::print("Book already open, restart engine if book file has changed\n");
    return;
  }

  const std::size_t size  = book_file.tellg();

  if (size <= sizeof(BookEntry))
  {
    fmt::print("Book format invalid. path={}\n", path);
    return;
  }

  const std::size_t count = size / sizeof(BookEntry);

  if (!count)
  {
    fmt::print("No entries located in book. path={}\n", path);
    return;
  }

  current_book_ = path.data();

  entries_.clear();
  entries_.reserve(count);

  book_file.seekg(0);
  for (std::size_t i = 0; i < count; i++)
  {
    BookEntry entry;
    book_file.read(reinterpret_cast<char*>(&entry), sizeof(BookEntry));
    entry.key    = std::byteswap(entry.key);
    entry.move   = std::byteswap(entry.move);
    entry.weight = std::byteswap(entry.weight);
    entry.learn  = std::byteswap(entry.learn);
    entries_.emplace_back(entry);
  }

  fmt::print("info string Parsed book. path={},size={}\n", path, entries_.size());

}

auto PolyBook::lower_entry(const std::uint64_t key) const
{
  const auto compare_lower = [](const BookEntry &entry, const std::uint64_t k) {
    return entry.key < k;
  };

  return std::lower_bound(entries_.begin(), entries_.end(), key, compare_lower);
}

auto PolyBook::upper_entry(const std::uint64_t key) const
{
  const auto compare_upper = [](const std::uint64_t k, const BookEntry &entry) {
    return k < entry.key;
  };

  return std::upper_bound(entries_.begin(), entries_.end(), key, compare_upper);
}

///
/// O(log(n) * m) lookup of known entries by key
///
Move PolyBook::probe(Board *board) const
{
  const auto key = poly_key(board);

  fmt::print("info string Probing book. key={}\n", key);

  const auto lower_boundry = lower_entry(key);

  if (lower_boundry != entries_.begin())
  {
    const auto e = *lower_boundry;
    if (e.key == key)
      return decode(board, e.move);
  }

  return MOVE_NONE;
}
