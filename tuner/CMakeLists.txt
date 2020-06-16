add_executable(FeliscatusTuner main.cpp)
target_link_libraries(FeliscatusTuner PRIVATE logic project_options project_warnings CONAN_PKG::fmt CONAN_PKG::docopt.cpp Threads::Threads)
