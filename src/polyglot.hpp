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

#include <vector>
#include <string_view>

#include "types.hpp"

struct Board;

struct PolyBook
{
  PolyBook() = default;

  void open(std::string_view path);

  Move probe(Board *board) const;

  std::size_t size() const;

  bool empty() const;

private:
  struct BookEntry
  {
    std::uint64_t key{};
    std::uint16_t move{};
    std::uint16_t weight{};
    std::uint32_t learn{};
  };

  using BookIterator = std::vector<BookEntry>::const_iterator;

  auto lower_entry(std::uint64_t key) const;
  auto upper_entry(std::uint64_t key, BookIterator lower_bound) const;
  auto select_random(BookIterator first, BookIterator second) const;

  std::string_view current_book_;
  std::vector<BookEntry> entries_;
};

inline std::size_t PolyBook::size() const
{
  return entries_.size();
}

inline bool PolyBook::empty() const
{
  return entries_.empty();
}

inline PolyBook book;
