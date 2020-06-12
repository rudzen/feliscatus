file(GLOB files
        "*.cpp"
        "*.h"
        )

#list(REMOVE_ITEM files "feliscatus.h")
list(REMOVE_ITEM files "feliscatus.cpp")

add_library(logic ${files})

add_executable(Feliscatus feliscatus.cpp)

target_link_libraries(logic PRIVATE project_options project_warnings CONAN_PKG::fmt Threads::Threads)

target_link_libraries(Feliscatus PRIVATE logic project_options project_warnings CONAN_PKG::fmt Threads::Threads)
