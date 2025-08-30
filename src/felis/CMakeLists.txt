# Copyright (C) 2008-2016 Gunnar Harms (Bobcat author)
# Copyright (C) 2017      FireFather (Tomcat author)
# Copyright (C) 2020-2025 Rudy Alex Kohn
# See end of file for extended copyright information.

set(files
        "arena.h"
        "bitboard.cpp"
        "bitboard.hpp"
        "board.cpp"
        "board.hpp"
        "cpu.cpp"
        "cpu.hpp"
        "eval.cpp"
        "eval.hpp"
        "hash.hpp"
        "material.cpp"
        "material.hpp"
        "miscellaneous.cpp"
        "miscellaneous.hpp"
        "move_gen.cpp"
        "moves.cpp"
        "moves.hpp"
        "parameters.hpp"
        "pawnhashtable.cpp"
        "pawnhashtable.hpp"
        "perft.cpp"
        "perft.hpp"
        "polyglot.cpp"
        "polyglot.hpp"
        "polyglot_keys.hpp"
        "position.cpp"
        "position.hpp"
        "prng.hpp"
        "score.hpp"
        "search.cpp"
        "search_limits.hpp"
        "see.cpp"
        "time.cpp"
        "time.hpp"
        "tpool.cpp"
        "tpool.hpp"
        "transpositional.cpp"
        "transpositional.hpp"
        "types.hpp"
        "uci.cpp"
        "uci_options.cpp"
        "uci.hpp"
        "util.hpp"
        "zobrist.hpp"
)

add_library(logic STATIC ${files})

target_link_libraries(logic PRIVATE project_options project_warnings fmt spdlog Threads::Threads)

# Feliscatus, a UCI chess playing engine derived from Tomcat 1.0 (Bobcat 8.0)
# Copyright (C) 2008-2016 Gunnar Harms (Bobcat author)
# Copyright (C) 2017      FireFather (Tomcat author)
# Copyright (C) 2020-2025 Rudy Alex Kohn
#
# Feliscatus is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Feliscatus is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Feliscatus.  If not, see <http://www.gnu.org/licenses/>.