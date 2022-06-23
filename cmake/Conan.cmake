macro(run_conan)
    # Download automatically, you can also just copy the conan.cmake file
    if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
        message(
                STATUS
                "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
        file(DOWNLOAD "https://github.com/conan-io/cmake-conan/raw/0.18.1/conan.cmake"
                "${CMAKE_BINARY_DIR}/conan.cmake")
    endif()

    include(${CMAKE_BINARY_DIR}/conan.cmake)

    conan_add_remote(NAME default URL
        https://center.conan.io)

            #blaze/3.7
            #docopt.cpp/0.6.2

    conan_cmake_run(
            REQUIRES
            ${CONAN_EXTRA_REQUIRES}
            catch2/3.0.1
            fmt/8.1.1
            spdlog/1.10.0
            nlohmann_json/3.10.5
            cli11/2.2.0
            OPTIONS
            ${CONAN_EXTRA_OPTIONS}
            BASIC_SETUP
            CMAKE_TARGETS # individual targets to link to
            BUILD
            missing)
endmacro()
