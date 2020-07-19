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

#include <cassert>

#include <fmt/format.h>

#include "uci.hpp"
#include "transpositional.hpp"
#include "util.hpp"
#include "tpool.hpp"

using std::string;

namespace {

constexpr std::array<std::string_view, 2> bool_string{"false", "true"};
constexpr int MaxHashMB            = 131072;
constinit std::size_t insert_order = 0;

}// namespace

namespace uci {

void on_clear_hash(const Option &) {
  TT.clear();
}

void on_hash_size(const Option &o) {
  TT.init(o);
}

void on_threads(const Option &o) {
  pool.set(o);
}

bool CaseInsensitiveLess::operator()(const std::string_view s1, const std::string_view s2) const noexcept {
  return std::lexicographical_compare(s1.begin(), s1.end(), s2.begin(), s2.end(), [](const char c1, const char c2) { return tolower(c1) < tolower(c2); });
}

void init(OptionsMap &o) {
  o[get_uci_name<UciOptions::THREADS>()] << Option(1, 1, 512, on_threads);
  o[get_uci_name<UciOptions::HASH>()] << Option(256, 1, MaxHashMB, on_hash_size);
  o[get_uci_name<UciOptions::HASH_X_THREADS>()] << Option(true);
  o[get_uci_name<UciOptions::CLEAR_HASH>()] << Option(on_clear_hash);
  o[get_uci_name<UciOptions::CLEAR_HASH_NEW_GAME>()] << Option(false);
  o[get_uci_name<UciOptions::PONDER>()] << Option(false);
  o[get_uci_name<UciOptions::UCI_Chess960>()] << Option(false);
  o[get_uci_name<UciOptions::SHOW_CPU>()] << Option(false);
}

/// Option class constructors and conversion operators

Option::Option(const char *v, const on_change f) : default_value_(v), current_value_(v), type_("string"), on_change_(f) {}

Option::Option(const bool v, const on_change f) : default_value_(bool_string[v]), current_value_(default_value_), type_("check"), on_change_(f) {}

Option::Option(const on_change f) : type_("button"), on_change_(f) {}

Option::Option(const int v, const int minv, const int maxv, const on_change f) : default_value_(fmt::format("{}", v)), current_value_(default_value_), type_("spin"), min_(minv), max_(maxv), on_change_(f) {}

Option::Option(const char *v, const char *cur, const on_change f) : default_value_(v), current_value_(cur), type_("combo"), on_change_(f) {}

Option::operator int() const {
  assert(type_ == "check" || type_ == "spin");
  return (type_ == "spin" ? util::to_integral<int>(current_value_) : current_value_ == bool_string[true]);
}

Option::operator std::string_view() const {
  assert(type_ == "string");
  return current_value_;
}

bool Option::operator==(const char *s) const {
  assert(type_ == "combo");
  return !CaseInsensitiveLess()(current_value_, s) && !CaseInsensitiveLess()(s, current_value_);
}

/// operator<<() inits options and assigns idx in the correct printing order

void Option::operator<<(const Option &o) {
  *this = o;
  idx_   = insert_order++;
}

/// operator=() updates currentValue and triggers on_change() action. It's up to
/// the GUI to check for option's limits, but we could receive the new value from
/// the user by console window, so let's check the bounds anyway.

Option &Option::operator=(const string &v) noexcept {

  assert(!type_.empty());

  if ((type_ != "button" && v.empty()) || (type_ == "check" && v != "true" && v != "false") || (type_ == "spin" && (!util::in_between(util::to_integral<int>(v), min_, max_))))
    return *this;

  if (type_ != "button")
    current_value_ = v;

  if (on_change_)
    on_change_(*this);

  return *this;
}

std::size_t Option::index() const noexcept {
  return idx_;
}

std::string_view Option::default_value() const noexcept {
  return default_value_;
}

std::string_view Option::current_value() const noexcept {
  return current_value_;
}

std::string_view Option::type() const noexcept {
  return type_;
}

int Option::min() const noexcept {
  return min_;
}

int Option::max() const noexcept {
  return max_;
}

}// namespace uci
