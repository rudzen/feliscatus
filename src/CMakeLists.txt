file(GLOB files
        "*.h"
		"*.hpp"
        "*.cpp"
        )

list(REMOVE_ITEM files "tomcat.h")
list(REMOVE_ITEM files "tomcat.cpp")

add_library(logic STATIC ${files})

add_executable(Feliscatus tomcat.cpp)

target_link_libraries(logic PRIVATE project_options project_warnings CONAN_PKG::fmt Threads::Threads)

target_link_libraries(Feliscatus PRIVATE logic project_options project_warnings CONAN_PKG::fmt Threads::Threads)
