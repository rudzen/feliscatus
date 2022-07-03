file(GLOB files
        "*.cpp"
        "*.hpp"
        )

add_library(io STATIC ${files})

target_link_libraries(io PRIVATE project_options project_warnings CONAN_PKG::fmt CONAN_PKG::spdlog)
