# Copyright (C) 2008-2016 Gunnar Harms (Bobcat author)
# Copyright (C) 2017      FireFather (Tomcat author)
# Copyright (C) 2020-2025 Rudy Alex Kohn
# See end of file for extended copyright information.

cmake_minimum_required(VERSION 4.0.2)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/exe/$<CONFIG>")

project(Feliscatus CXX)
include(cmake/StandardProjectSettings.cmake)
include(cmake/PreventInSourceBuilds.cmake)

# Link this 'library' to set the c++ standard / compile-time options requested
add_library(project_options INTERFACE)
target_compile_features(project_options INTERFACE cxx_std_23)

if(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
    option(ENABLE_BUILD_WITH_TIME_TRACE "Enable -ftime-trace to generate time tracing .json files on clang" OFF)
    if (ENABLE_BUILD_WITH_TIME_TRACE)
        add_compile_definitions(project_options INTERFACE -ftime-trace)
    endif()
endif()

# Link this 'library' to use the warnings specified in CompilerWarnings.cmake
add_library(project_warnings INTERFACE)

# enable cache system
include(cmake/Cache.cmake)

# standard compiler warnings
include(cmake/CompilerWarnings.cmake)
set_project_warnings(project_warnings)

# sanitizer options if supported by compiler
include(cmake/Sanitizers.cmake)
enable_sanitizers(project_options)

# enable doxygen
include(cmake/Doxygen.cmake)
enable_doxygen()

# allow for static analysis options
include(cmake/StaticAnalyzers.cmake)

option(BUILD_SHARED_LIBS "Enable compilation of shared libraries" OFF)
option(ENABLE_TESTING "Enable Test Builds" OFF)
option(ENABLE_FUZZING "Enable Fuzzing Builds" OFF)

# Very basic PCH example
option(ENABLE_PCH "Enable Precompiled Headers" OFF)
if (ENABLE_PCH)
    # This sets a global PCH parameter, each project will build its own PCH, which
    # is a good idea if any #define's change
    #
    # consider breaking this out per project as necessary
    target_precompile_headers(project_options INTERFACE <vector> <string> <map> <utility>)
endif()

include(cmake/CPM.cmake)

CPMAddPackage("gh:fmtlib/fmt#11.2.0")
CPMAddPackage(
        NAME
        spdlog
        VERSION
        1.15.3
        GITHUB_REPOSITORY
        "gabime/spdlog"
        OPTIONS
        "SPDLOG_FMT_EXTERNAL ON")

CPMAddPackage("gh:CLIUtils/CLI11@2.5.0")
CPMAddPackage(
        URI "gh:nlohmann/json@3.12.0"
        OPTIONS "JSON_BuildTests OFF"
)

# include engine options
include(cmake/Engine.cmake)
enable_engine_options()

#if(ENABLE_TESTING)
#  enable_testing()
#  message("Building Tests.")
#  add_subdirectory(test)
#endif()

if(ENABLE_FUZZING)
  message("Building Fuzz Tests, using fuzzing sanitizer https://www.llvm.org/docs/LibFuzzer.html")
  add_subdirectory(fuzz_test)
endif()

add_subdirectory(src)

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