file(GLOB files
        "*.cpp"
        "*.h"
        )

list(REMOVE_ITEM files "main.cpp")

include(FetchContent)
FetchContent_Declare(
  cli11
  GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
)

FetchContent_GetProperties(cli11)
if(NOT cli11_POPULATED)
  FetchContent_Populate(cli11)
  add_subdirectory(${cli11_SOURCE_DIR} ${cli11_BINARY_DIR})
endif()

add_library(tune STATIC ${files})

add_executable(FeliscatusTuner main.cpp)

target_link_libraries(tune PRIVATE logic project_options project_warnings CONAN_PKG::fmt CONAN_PKG::spdlog CONAN_PKG::nlohmann_json CLI11 Threads::Threads)

target_link_libraries(FeliscatusTuner PRIVATE logic tune project_options project_warnings CONAN_PKG::fmt CONAN_PKG::spdlog CONAN_PKG::nlohmann_json CLI11 Threads::Threads)
