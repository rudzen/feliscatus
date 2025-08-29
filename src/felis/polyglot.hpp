// Copyright (C) 2008-2016 Gunnar Harms (Bobcat author)
// Copyright (C) 2017      FireFather (Tomcat author)
// Copyright (C) 2020-2025 Rudy Alex Kohn
// See end of file for extended copyright information.

#pragma once

#include <felis/arena.h>
#include <felis/types.hpp>

struct Board;
struct BookEntry;

struct PolyBook
{
  PolyBook() = default;

  void open(std::string_view path);

  Move probe(Board *board) const;

  std::size_t size() const;

  bool empty() const;

private:
  BookEntry* lower_entry(std::uint64_t key) const;
  BookEntry* upper_entry(std::uint64_t key, BookEntry* lower_bound) const;
  BookEntry* select_random(BookEntry* first, const BookEntry * second) const;

  const char* book_name;
  BookEntry* entries;
  size_t entry_count;

  static Arena arena;
};

inline std::size_t PolyBook::size() const
{
  return entry_count;
}

inline bool PolyBook::empty() const
{
  return entry_count == 0;
}

inline PolyBook book;

// Feliscatus, a UCI chess playing engine derived from Tomcat 1.0 (Bobcat 8.0)
// Copyright (C) 2008-2016 Gunnar Harms (Bobcat author)
// Copyright (C) 2017      FireFather (Tomcat author)
// Copyright (C) 2020-2022 Rudy Alex Kohn
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