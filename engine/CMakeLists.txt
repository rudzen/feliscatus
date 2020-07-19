
add_executable(Feliscatus main.cpp)

target_link_libraries(Feliscatus PRIVATE logic project_options project_warnings CONAN_PKG::fmt CONAN_PKG::spdlog Threads::Threads)
