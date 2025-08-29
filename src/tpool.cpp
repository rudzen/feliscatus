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

#include <numeric>
#include <execution>

#include <tpool.hpp>
#include <uci.hpp>
#include <board.hpp>
#include <transpositional.hpp>
#include <search_limits.hpp>

namespace
{

// Calculate required memory for thread objects
constexpr size_t calculate_thread_memory_requirement()
{
  // Basic thread object size
  constexpr size_t thread_base_size = sizeof(struct thread);
  constexpr size_t main_thread_size = sizeof(struct main_thread);

  // Board object size (allocated separately per thread)
  constexpr size_t board_size = sizeof(Board);

  // Add some padding for alignment
  constexpr size_t alignment_padding = 64;

  return std::max(thread_base_size, main_thread_size) + board_size + alignment_padding;
}

constexpr size_t THREAD_MEMORY_SIZE = calculate_thread_memory_requirement();
constexpr size_t PARALLEL_THRESHOLD = 8;

/**
 * Reset and prepare arena for thread allocation based on thread count
 * Calculates required memory for threads and boards, resets arena state
 */
void reset_thread_arena(Arena &arena, size_t thread_count)
{
  // Calculate total memory needed for all threads
  // Main thread (slightly larger) + regular threads + boards + alignment padding
  const size_t total_memory_needed = thread_count * THREAD_MEMORY_SIZE;

  // Add memory for SearchLimits and its search_moves array
  const size_t search_limits_size = sizeof(SearchLimits) + (sizeof(Move) * MAX_MOVES);

  // Add extra space for safety (20% overhead)
  const size_t arena_size = total_memory_needed + search_limits_size + ((total_memory_needed + search_limits_size) / 5);

  // Check if current arena capacity is sufficient
  if (arena.capacity() < arena_size)
  {
    // Need to resize the arena - calculate new size with some growth factor
    // Use at least 50% more than required to avoid frequent resizing
    const size_t new_capacity = arena_size + (arena_size / 2);

    if (!arena.resize_and_reset(new_capacity))
    {
      // Failed to resize arena - this is a critical error
      // For now, we'll continue with existing capacity and hope it works
      // In a production system, you might want to throw an exception or handle this differently
      arena.reset();
    }
  } else   // Arena has sufficient capacity, just reset it
    arena.reset();
}

/**
 * Allocate and construct a thread object from arena memory
 * Uses placement new to properly construct objects
 * Returns nullptr if out of memory
 */
template<typename ThreadType>
ThreadType *allocate_thread_from_arena(Arena &arena, size_t index)
{
  // Allocate thread object from arena
  ThreadType *thread_obj = arena.allocate<ThreadType>(1);
  if (!thread_obj)
    return nullptr;   // Out of memory

  // Allocate Board object from arena
  Board *board_obj = arena.allocate<Board>(1);
  if (!board_obj)
    return nullptr;   // Out of memory - arena will be reset anyway

  // Use placement new to construct the thread object
  new (thread_obj) ThreadType(index);

  // Use placement new to construct the Board object
  // TODO (rudzen) : This should be removed once the board class is refactored to not require a constructor
  new (board_obj) Board();

  // Directly assign the arena-allocated Board pointer
  thread_obj->root_board = board_obj;

  return thread_obj;
}

}   // namespace

thread::thread(const size_t index) : jthread(&thread::idleLoop, this), idx(index), searching(true)
{ }

thread::~thread()
{
  assert(!searching.load());

  exit.store(true);
  start_searching();
}

void thread::clearData()
{
  std::memset(history_scores.data(), 0, sizeof history_scores);
  std::memset(counter_moves.data(), 0, sizeof counter_moves);
  pv_length.fill(0);
  pv.fill({});
  draw_score.fill(0);
}

void thread::idleLoop()
{
  // NUMA fix
  if (Options[uci::uciName<uci::UciOptions::THREADS>()] > 8)
    WinProcGroup::bind_this_thread(idx);

  do
  {
    std::unique_lock lk(mutex);
    searching.store(false);

    // Wake up anyone waiting for search finished
    cv.notify_one();
    cv.wait(lk, [&] {
      return searching.load(std::memory_order_relaxed);
    });

    // check exit flag, this is set when the class is being destroyed
    [[unlikely]]
    if (exit.load())
      break;

    lk.unlock();

    search();
  } while (true);
}

void thread::start_searching()
{
  std::lock_guard lk(mutex);
  searching.store(true);
  cv.notify_one();   // Wake up the thread in idleLoop()
}

void thread::wait_for_search_finished()
{
  std::unique_lock lk(mutex);
  cv.wait(lk, [&] {
    return !searching.load();
  });
}

#if defined(linux)
thread_pool::thread_pool()
#else
thread_pool::thread_pool()
  : node_counters(
      {[&] {
         return node_count_seq();
       },
       [&] {
         return node_count_par();
       }})
#endif
{
  // Don't allocate from arena during constructor - the arena will be reset in set()
  // Initialize limits as nullptr, it will be allocated in set()
  limits = nullptr;
}

void thread_pool::set(const size_t v)
{
  while (!empty())
    pop_back();

  assert(v > 0);

  if (v == 0)
    return;

  // Reset and prepare the arena for thread allocation
  reset_thread_arena(thread_arena, v);

  // Allocate SearchLimits from arena first
  limits = thread_arena.allocate<SearchLimits>(1);
  if (limits) {
    // Allocate the search_moves array from arena
    limits->search_moves = thread_arena.allocate<Move>(MAX_MOVES);
    if (limits->search_moves) {
      // Initialize the SearchLimits structure
      ClearSearchLimits(limits);
    }
  }

  // Allocate main thread from arena
  main_thread *main_t = allocate_thread_from_arena<main_thread>(thread_arena, 0);
  if (main_t)
    emplace_back(main_t);

  // Allocate remaining threads from arena
  while (size() < v)
  {
    thread *t = allocate_thread_from_arena<thread>(thread_arena, size());
    if (t)
      emplace_back(t);
  }

  clear_data();

  size_t tt_size = Options[uci::uciName<uci::UciOptions::HASH>()];

  if (Options[uci::uciName<uci::UciOptions::HASH_X_THREADS>()])
    tt_size *= size();

  TT.init(tt_size);

#if !defined(linux)
  parallel = size() > PARALLEL_THRESHOLD;
#endif
}

void thread_pool::start_thinking(std::string_view fen)
{
  auto *front_thread = main();

  front_thread->wait_for_search_finished();

  stop                 = false;
  front_thread->ponder = limits->ponder;

  const auto setup = [&fen](thread *t) {
    t->node_count = 0;
    t->root_board->set_fen(fen, t);
  };

#if defined(linux)
  std::for_each(begin(), end(), setup);
#else
  std::for_each(std::execution::par_unseq, begin(), end(), setup);
#endif

  front_thread->start_searching();
}

void thread_pool::start_searching()
{
  auto start = [](thread *t) {
    t->start_searching();
  };
  std::for_each(std::next(begin()), end(), start);
}

void thread_pool::wait_for_search_finished()
{
  auto wait = [](thread *t) {
    t->wait_for_search_finished();
  };
  std::for_each(std::next(begin()), end(), wait);
}

void thread_pool::clear_data() const
{
  for (auto &w : *this)
    w->clearData();
}

// Initialize the static thread arena with a reasonable size (32MB)
Arena thread_pool::thread_arena(32 * 1024 * 1024);

u64 thread_pool::node_count() const
{
#if defined(linux)
  const auto accumulator = [](const u64 r, const thread *d) {
    return r + d->node_count.load(std::memory_order_relaxed);
  };
  return std::accumulate(cbegin(), cend(), 0ull, accumulator);
#else
  return node_counters[parallel]();
#endif
}

#if !defined(linux)
u64 thread_pool::node_count_seq() const
{
  const auto accumulator = [](const u64 r, const thread *d) {
    return r + d->node_count.load(std::memory_order_relaxed);
  };
  return std::accumulate(cbegin(), cend(), 0ull, accumulator);
}

u64 thread_pool::node_count_par() const
{
  const auto accumulator = [](const thread *d) {
    return d->node_count.load(std::memory_order_relaxed);
  };
  return std::transform_reduce(std::execution::par_unseq, cbegin(), cend(), 0ull, std::plus(), accumulator);
}
#endif
