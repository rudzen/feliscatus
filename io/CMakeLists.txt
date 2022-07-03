file(GLOB files
        "*.cpp"
        "*.hpp"
        )

add_library(io STATIC ${files})

configure_file (
        "${CMAKE_CURRENT_SOURCE_DIR}/Feliscatus.json"
        "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Feliscatus.json"
        COPYONLY)

target_link_libraries(io PRIVATE project_options project_warnings CONAN_PKG::fmt CONAN_PKG::spdlog CONAN_PKG::nlohmann_json)
