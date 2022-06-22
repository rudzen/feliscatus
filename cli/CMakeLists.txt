file(GLOB files
        "*.cpp"
        "*.hpp"
        )

add_library(cli STATIC ${files})

target_link_libraries(cli PRIVATE project_options project_warnings CONAN_PKG::cli11)
