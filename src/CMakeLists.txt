file(GLOB files
        "*.cpp"
        "*.hpp"
        )

add_library(logic STATIC ${files})

target_link_libraries(logic PRIVATE project_options project_warnings fmt spdlog Threads::Threads)
