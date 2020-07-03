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

#include <chrono>

#include "miscellaneous.h"

struct Stopwatch final {

  Stopwatch() : start_time_(std::chrono::system_clock::now()), running_(true) { }

  void start() {
    start_time_ = std::chrono::system_clock::now();
    running_  = true;
  }

  void stop() {
    end_time_  = std::chrono::system_clock::now();
    running_ = false;
  }

  [[nodiscard]]
  TimeUnit elapsed_milliseconds() const {
    const auto end_time = running_ ? std::chrono::system_clock::now() : end_time_;
    return std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time_).count();
  }

  [[nodiscard]]
  TimeUnit elapsed_microseconds() const {
    const auto end_time = running_ ? std::chrono::system_clock::now() : end_time_;
    return std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time_).count();
  }

  [[nodiscard]]
  TimeUnit elapsed_seconds() const { return elapsed_milliseconds() / 1000; }

private:
  std::chrono::time_point<std::chrono::system_clock> start_time_;
  std::chrono::time_point<std::chrono::system_clock> end_time_;
  std::chrono::time_point<std::chrono::system_clock> last_curr_info_;
  bool running_;
};
