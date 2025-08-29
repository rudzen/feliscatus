set(files
        "cli_parser.cpp"
        "cli_parser.hpp"
)

add_library(cli STATIC ${files})

target_link_libraries(cli PRIVATE project_options project_warnings CLI11)
