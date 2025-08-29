set(files
        "directory_resolver.cpp"
        "directory_resolver.hpp"
        "file_resolver.cpp"
        "file_resolver.hpp"
        "settings_resolver.cpp"
        "settings_resolver.hpp"
        )

add_library(io STATIC ${files})
target_include_directories(io PUBLIC .)

configure_file (
        "${CMAKE_CURRENT_SOURCE_DIR}/Feliscatus.json"
        "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Feliscatus.json"
        COPYONLY)


target_link_libraries(io PRIVATE project_options project_warnings fmt spdlog nlohmann_json)
