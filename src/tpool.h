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

#include <array>
#include <atomic>
#include <memory>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <string_view>
#include <vector>
#include <functional>

#include "pawnhashtable.h"
#include "pv_entry.h"
#include "timemanager.h"
#include "types.h"

/// Main thread pool header
/// Contains pool, thread and main_thread

using HistoryScores = std::array<std::array<int, SQ_NB>, 16>;
using CounterMoves  = std::array<std::array<Move, SQ_NB>, 16>;

struct thread {

  explicit thread(std::size_t index);
  virtual ~thread();
  thread(const thread &other)       = delete;
  thread(thread &&other)            = delete;
  thread &operator=(const thread &) = delete;
  thread &operator=(thread &&other) = delete;

  virtual void search();
  void clear_data();
  void idle_loop();
  void start_searching();
  void wait_for_search_finished();

  [[nodiscard]]
  std::size_t index() const { return idx; }

  PawnHashTable pawn_hash{};
  HistoryScores history_scores{};
  CounterMoves counter_moves{};
  std::array<std::array<PVEntry, MAXDEPTH>, MAXDEPTH> pv{};
  std::array<int, MAXDEPTH> pv_length{};
  std::atomic_uint64_t node_count;
  std::condition_variable waiter;
  std::unique_ptr<Board> root_board{};
  std::array<int, COL_NB> draw_score{};

private:
  std::mutex mutex;
  std::condition_variable cv;
  std::size_t idx;
  std::jthread jthread;
  bool exit{};
  bool searching;
};

struct main_thread final : thread {
  using thread::thread;

  void search() override;

  std::atomic_bool ponder;
  TimeManager time{};
};

struct thread_pool : std::vector<std::unique_ptr<thread>> {

  thread_pool();
  ~thread_pool()                              = default;
  thread_pool(const thread_pool &other)       = delete;
  thread_pool(thread_pool &&other)            = delete;
  thread_pool &operator=(const thread_pool &) = delete;
  thread_pool &operator=(thread_pool &&other) = delete;

  void set(std::size_t v);

  void start_thinking(std::string_view fen);
  void start_searching();
  void wait_for_search_finished();

  [[nodiscard]]
  main_thread *main() const { return static_cast<main_thread *>(front().get()); }

  void clear_data();

  [[nodiscard]]
  uint64_t node_count() const;

  SearchLimits limits{};
  std::atomic_bool stop;

private:
  [[nodiscard]]
  uint64_t node_count_par() const;

  [[nodiscard]]
  uint64_t node_count_seq() const;

  bool parallel{};
  const std::array<std::function<uint64_t()>, 2> node_counters;
};

// global data object
inline thread_pool pool;
