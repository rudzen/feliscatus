# Copyright (C) 2008-2016 Gunnar Harms (Bobcat author)
# Copyright (C) 2017      FireFather (Tomcat author)
# Copyright (C) 2020-2025 Rudy Alex Kohn
# See end of file for extended copyright information.

set(files
        "cli_parser.cpp"
        "cli_parser.hpp"
)

add_library(cli STATIC ${files})

target_link_libraries(cli PRIVATE project_options project_warnings CLI11)

# Feliscatus, a UCI chess playing engine derived from Tomcat 1.0 (Bobcat 8.0)
# Copyright (C) 2008-2016 Gunnar Harms (Bobcat author)
# Copyright (C) 2017      FireFather (Tomcat author)
# Copyright (C) 2020-2022 Rudy Alex Kohn
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