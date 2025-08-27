
add_executable(Feliscatus main.cpp)

target_link_libraries(Feliscatus PRIVATE logic io project_options project_warnings fmt spdlog Threads::Threads)
