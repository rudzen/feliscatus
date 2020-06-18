file(GLOB files
        "*.cpp"
        "*.h"
        )

list(REMOVE_ITEM files "main.cpp")

add_library(tune STATIC ${files})

add_executable(FeliscatusTuner main.cpp)

target_link_libraries(tune PRIVATE logic project_options project_warnings CONAN_PKG::fmt CONAN_PKG::docopt.cpp CONAN_PKG::spdlog Threads::Threads)

target_link_libraries(FeliscatusTuner PRIVATE logic tune project_options project_warnings CONAN_PKG::fmt CONAN_PKG::docopt.cpp CONAN_PKG::spdlog Threads::Threads)
