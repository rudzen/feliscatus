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

#include <numeric>
#include <execution>

#include "tpool.h"
#include "uci.h"
#include "board.h"
#include "transpositional.h"

namespace {
  constexpr std::size_t parallel_threshold = 8;
}

thread::thread(const std::size_t index) : root_board(std::make_unique<Board>()), idx(index), jthread(&thread::idle_loop, this), searching(true) {}

thread::~thread() {
  assert(!searching);

  exit = true;
  start_searching();
}

void thread::clear_data() {
  std::memset(history_scores.data(), 0, sizeof history_scores);
  std::memset(counter_moves.data(), 0, sizeof counter_moves);
  pv_length.fill(0);
  pv.fill({});
  draw_score.fill(0);
}

void thread::idle_loop() {
  // NUMA fix
  if (Options[uci::get_uci_name<uci::UciOptions::THREADS>()] > 8)
    WinProcGroup::bind_this_thread(idx);

  do
  {
    std::unique_lock<std::mutex> lk(mutex);
    searching = false;

    // Wake up anyone waiting for search finished
    cv.notify_one();
    cv.wait(lk, [&] { return searching; });

    // check exit flag, this is set when the class is destroyed
    if (exit)
      return;

    lk.unlock();

    search();
  } while (true);
}

void thread::start_searching() {
  std::lock_guard<std::mutex> lk(mutex);
  searching = true;
  cv.notify_one();// Wake up the thread in idle_loop()
}

void thread::wait_for_search_finished() {
  std::unique_lock<std::mutex> lk(mutex);
  cv.wait(lk, [&] { return !searching; });
}

thread_pool::thread_pool() : node_counters({[&] { return node_count_seq(); }, [&] { return node_count_par(); }}) {}

void thread_pool::set(const std::size_t v) {
  while (!empty())
    pop_back();

  assert(v > 0);

  if (v > 0)
  {
    emplace_back(std::make_unique<main_thread>(0));

    while (size() < v)
      emplace_back(std::make_unique<thread>(size()));

    clear_data();

    auto tt_size = static_cast<std::size_t>(Options[uci::get_uci_name<uci::UciOptions::HASH>()]);

    if (Options[uci::get_uci_name<uci::UciOptions::HASH_X_THREADS>()])
      tt_size *= size();

    TT.init(tt_size);

    parallel = size() > parallel_threshold;
  }
}

void thread_pool::start_thinking(std::string_view fen) {

  auto *front_thread = main();

  front_thread->wait_for_search_finished();

  stop                 = false;
  front_thread->ponder = limits.ponder;

  const auto setup = [&](std::unique_ptr<thread> &t) {
    t->node_count = 0;
    t->root_board->set_fen(fen, t.get());
  };

  std::for_each(std::execution::par_unseq, begin(), end(), setup);

  front_thread->start_searching();
}

void thread_pool::start_searching() {
  auto start = [](std::unique_ptr<thread> &t) { t->start_searching(); };
  std::for_each(std::next(begin()), end(), start);
}

void thread_pool::wait_for_search_finished() {
  auto wait = [](std::unique_ptr<thread> &t) { t->wait_for_search_finished(); };
  std::for_each(std::next(begin()), end(), wait);
}

void thread_pool::clear_data() {
  for (auto &w : *this)
    w->clear_data();
}

uint64_t thread_pool::node_count() const {
  return node_counters[parallel]();
}

uint64_t thread_pool::node_count_seq() const {
  const auto accumulator = [](const uint64_t r, const std::unique_ptr<thread> &d) { return r + d->node_count.load(std::memory_order_relaxed); };
  return std::accumulate(cbegin(), cend(), 0ull, accumulator);
}

uint64_t thread_pool::node_count_par() const {
  const auto accumulator = [](const std::unique_ptr<thread> &d) { return d->node_count.load(std::memory_order_relaxed); };
  return std::transform_reduce(std::execution::par_unseq, cbegin(), cend(), 0ull, std::plus<>(), accumulator);
}
