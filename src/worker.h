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

#include <thread>
#include <memory>
#include <string_view>

#include "board.h"
#include "search.h"
#include "uci.h"
#include "miscellaneous.h"

struct Worker final {

  Worker() : board_(std::make_unique<Board>()) {}

  void start(std::string_view fen, const std::size_t index) {
    board_->set_fen(fen);
    search_ = std::make_unique<Search>(board_.get(), index);
    thread_ = std::jthread(&Search::run, search_.get());
    if (Options[uci::get_uci_name<uci::UciOptions::THREADS>()] > 8)
      WinProcGroup::bind_this_thread(index);
  }

  void stop() {
    if (search_)
      search_->stop();
    if (thread_.joinable())
    {
      thread_.request_stop();
      thread_.join();
    }
  }

private:
  std::unique_ptr<Board> board_{};
  std::unique_ptr<Search> search_{};
  std::jthread thread_{};
};