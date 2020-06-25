file(GLOB files
        "*.cpp"
        "*.h"
        )

list(REMOVE_ITEM files "main.cpp")

add_library(logic STATIC ${files})

add_executable(Feliscatus main.cpp)

target_link_libraries(logic PRIVATE project_options project_warnings CONAN_PKG::fmt CONAN_PKG::spdlog Threads::Threads)

target_link_libraries(Feliscatus PRIVATE logic project_options project_warnings CONAN_PKG::fmt CONAN_PKG::spdlog Threads::Threads)
