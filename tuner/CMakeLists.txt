file(GLOB files
        "*.cpp"
        "*.hpp"
        )

add_definitions("-DTUNER")

list(REMOVE_ITEM files "main.cpp")

add_library(tune STATIC ${files})

add_executable(FeliscatusTuner main.cpp)

target_link_libraries(tune PRIVATE logic CONAN_PKG::cli11 cli project_options project_warnings CONAN_PKG::fmt CONAN_PKG::spdlog CONAN_PKG::nlohmann_json Threads::Threads)

target_link_libraries(FeliscatusTuner PRIVATE logic CONAN_PKG::cli11 cli tune project_options project_warnings CONAN_PKG::fmt CONAN_PKG::spdlog CONAN_PKG::nlohmann_json Threads::Threads)
