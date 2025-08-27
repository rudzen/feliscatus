file(GLOB files
        "*.cpp"
        "*.hpp"
        "*.h"
        )

add_definitions("-DTUNER")

list(REMOVE_ITEM files "main.cpp")

add_library(tune STATIC ${files})

add_executable(FeliscatusTuner main.cpp)

target_link_libraries(tune PRIVATE logic CLI11 cli project_options project_warnings fmt spdlog nlohmann_json Threads::Threads)

target_link_libraries(FeliscatusTuner PRIVATE logic CLI11 cli tune project_options project_warnings fmt spdlog nlohmann_json Threads::Threads)
