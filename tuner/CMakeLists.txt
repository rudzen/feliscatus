file(GLOB files
        "*.cpp"
        "*.hpp"
        )

list(REMOVE_ITEM files "main.cpp")

add_library(tune STATIC ${files})

add_executable(FeliscatusTuner main.cpp)

target_link_libraries(tune PRIVATE logic cli project_options project_warnings CONAN_PKG::fmt CONAN_PKG::spdlog CONAN_PKG::nlohmann_json Threads::Threads)

target_link_libraries(FeliscatusTuner PRIVATE logic cli tune project_options project_warnings CONAN_PKG::fmt CONAN_PKG::spdlog CONAN_PKG::nlohmann_json Threads::Threads)
