// Copyright (C) 2008-2016 Gunnar Harms (Bobcat author)
// Copyright (C) 2017      FireFather (Tomcat author)
// Copyright (C) 2020-2025 Rudy Alex Kohn
// See end of file for extended copyright information.

#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <numeric>
#include <chrono>
#include <fmt/format.h>
#include <felis/polyglot.hpp>
#include <felis/polyglot_keys.hpp>
#include <felis/board.hpp>
#include <felis/bitboard.hpp>
#include <felis/moves.hpp>
#include <felis/uci.hpp>
#include <felis/prng.hpp>

namespace {

constexpr std::array poly_castles{WHITE_OO, WHITE_OOO, BLACK_OO, BLACK_OOO};

constexpr u64 get_piece_key(const Piece pc, const Square sq) {
  return Polyglot::Keys::pc_key(pc, sq);
}

constexpr u64 get_castle_key(const CastlingRight cr) {
  const CastlingRight* const end = std::find(poly_castles.cbegin(), poly_castles.cend(), cr);
  const std::ptrdiff_t idx       = std::distance(poly_castles.cbegin(), end);
  return Polyglot::Keys::castle_key(idx);
}

constexpr u64 get_side_key(const Color c) {
  if (c == WHITE)
    return Polyglot::Keys::side_key();

  return 0ULL;
}

constexpr u64 get_en_passant_key(const File f) {
  return Polyglot::Keys::en_passant_key(f);
}

u64 hash_pieces(const Board* board) {
  u64 hash{};
  Bitboard pieces = board->pieces();

  while (pieces) {
    const Square sq = pop_lsb(&pieces);
    const Piece pc  = board->piece(sq);
    hash ^= get_piece_key(pc, sq);
  }

  return hash;
}

u64 hash_castle(Board* board) {
  if (!board->can_castle())
    return 0ULL;

  const auto accumulator = [&board](const u64 r, const CastlingRight cr) { return board->can_castle(cr) ? r ^ get_castle_key(cr) : r; };

  return std::accumulate(poly_castles.cbegin(), poly_castles.cend(), 0ULL, accumulator);
}

u64 hash_enpassant(const Square ep_sq) {
  if (ep_sq != NO_SQ)
    return get_en_passant_key(file_of(ep_sq));

  return 0;
}

u64 hash_turn(const Color stm) {
  return get_side_key(stm);
}

u64 poly_key(Board* board) {
  return hash_pieces(board) ^ hash_castle(board) ^ hash_turn(board->side_to_move()) ^ hash_enpassant(board->en_passant_square());
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
Move decode(Board* board, const u16 move) {
  if (!move)
    return MOVE_NONE;

  const File to_f          = static_cast<File>(move & 0x7);
  const Rank to_r          = static_cast<Rank>((move & 0x38) >> 3);
  const File from_f        = static_cast<File>((move & 0x1C0) >> 6);
  const Rank from_r        = static_cast<Rank>((move & 0xE00) >> 9);
  const PieceType promoted = static_cast<PieceType>((move & 0x7000) >> 12);
  const Square from        = make_square(from_f, from_r);
  const Square to          = make_square(to_f, to_r);
  const Piece pc           = board->piece(from);
  const PieceType pt       = type_of(pc);

  // check castleling move

  if (pt == KING) {
    if (from == E1) {
      if (to == H1)
        return init_move<CASTLE>(pc, NO_PIECE, from, G1, NO_PIECE);

      if (to == A1)
        return init_move<CASTLE>(pc, NO_PIECE, from, A1, NO_PIECE);
    } else if (from == E8) {
      if (to == H8)
        return init_move<CASTLE>(pc, NO_PIECE, from, G8, NO_PIECE);

      if (to == A8)
        return init_move<CASTLE>(pc, NO_PIECE, from, C8, NO_PIECE);
    }
  }

  const MoveList<LEGALMOVES> ml = MoveList<LEGALMOVES>(board);

  const auto from_to_matches = [&from, &to](const Move m) { return from == move_from(m) && to == move_to(m); };

  const MoveData* const m = std::find_if(ml.cbegin(), ml.cend(), from_to_matches);

  if (m != ml.end()) {
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

struct BookEntry {
  std::uint64_t key;
  std::uint16_t move;
  std::uint16_t weight;
  std::uint32_t learn;
};

// Initialize the static thread arena with a reasonable size (8MB)
Arena PolyBook::arena(8 * 1024 * 1024);

///
/// Opens a binary polyglot book and parses entries
///
void PolyBook::open(const std::string_view path) {
  fmt::print("{}", "info string Loading book...\n");

  std::ifstream book_file = std::ifstream(path.data(), std::ios::binary | std::ios::ate);

  if (!book_file) {
    fmt::print("Unable to open book. path={}\n", path);
    return;
  }

  if (book_name && book_name == path) {
    fmt::print("Book already open, restart engine if book file has changed\n");
    return;
  }

  const std::size_t size = book_file.tellg();

  if (size <= sizeof(BookEntry)) {
    fmt::print("Book format invalid. path={}\n", path);
    return;
  }

  const std::size_t count = size / sizeof(BookEntry);

  if (!count) {
    fmt::print("No entries located in book. path={}\n", path);
    return;
  }

  const size_t arena_required = count * sizeof(BookEntry);

  if (arena.capacity() < arena_required)
    arena.resize_and_reset(arena_required);
  else if (arena.remaining() < arena_required)
    arena.reset();

  entries     = arena.allocate<BookEntry>(count);
  entry_count = count;

  std::memset(entries, 0, count * sizeof(BookEntry));

  book_name = path.data();

  book_file.seekg(0);

  BookEntry* entry = entries;

  for (std::size_t i = 0; i < count; i++) {
    book_file.read(reinterpret_cast<char*>(entry), sizeof(BookEntry));
    entry->key    = std::byteswap(entry->key);
    entry->move   = std::byteswap(entry->move);
    entry->weight = std::byteswap(entry->weight);
    entry->learn  = std::byteswap(entry->learn);
    entry++;
  }

  fmt::print("info string Parsed book. path={},size={},entries={}\n", path, size, count);
}

BookEntry* PolyBook::lower_entry(const u64 key) const {
  const auto compare_lower = [](const BookEntry& entry, const u64 k) { return entry.key < k; };

  return std::lower_bound(entries, entries + entry_count, key, compare_lower);
}

BookEntry* PolyBook::upper_entry(const u64 key, BookEntry* lower_bound) const {
  const auto compare_upper = [](const u64 k, const BookEntry& entry) { return k < entry.key; };

  return std::upper_bound(lower_bound, entries + entry_count, key, compare_upper);
}

BookEntry* PolyBook::select_random(BookEntry* first, const BookEntry* second) const {
  u16 max_weight                                                         = 0;
  std::size_t sum_weight                                                 = 0;
  const std::chrono::duration<long long, std::ratio<1, 1000000000>> seed = std::chrono::system_clock::now().time_since_epoch();
  PRNG rng(seed.count());

  BookEntry* selected = first;
  for (BookEntry* it = first; it != second; it = std::next(it)) {
    max_weight = std::max(first->weight, max_weight);
    sum_weight += max_weight;

    if (sum_weight != 0 && rng.rand<std::size_t>() % sum_weight < first->weight)
      selected = it;
  }

  return selected;
}

/// O(log(n)) lookup of known entries by key
Move PolyBook::probe(Board* board) const {
  const auto key = poly_key(board);

  fmt::print("info string Probing book. key={}\n", key);

  BookEntry* const lower_boundry = lower_entry(key);

  if (lower_boundry == entries)
    return MOVE_NONE;

  const BookEntry* e;

  // In case we have set best book move,
  // we don't have to look any further
  if (Options[uci::uciName<uci::UciOptions::BOOK_BEST_MOVE>()])
    e = lower_boundry;
  else {
    BookEntry* const upper_boundry  = upper_entry(key, lower_boundry);
    const std::ptrdiff_t move_count = std::distance(lower_boundry, upper_boundry);

    if (move_count == 1)
      e = lower_boundry;
    else if (upper_boundry != entries + entry_count) {
      e = select_random(lower_boundry, upper_boundry);

      const auto s = uci::info(fmt::format("number of book moves = {}\n", move_count));
      fmt::print("{}", s);
    } else
      e = lower_boundry;
  }

  return e && e->key == key ? decode(board, e->move) : MOVE_NONE;
}

// Feliscatus, a UCI chess playing engine derived from Tomcat 1.0 (Bobcat 8.0)
// Copyright (C) 2008-2016 Gunnar Harms (Bobcat author)
// Copyright (C) 2017      FireFather (Tomcat author)
// Copyright (C) 2020-2025 Rudy Alex Kohn
//
// Feliscatus is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Feliscatus is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Feliscatus.  If not, see <http://www.gnu.org/licenses/>.