# Set a default build type if none was specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to 'RelWithDebInfo' as none was specified.")
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Choose the type of build." FORCE)
    # Set the possible values of build type for cmake-gui, ccmake
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# Generate compile_commands.json to make it easier to work with clang based
# tools
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(ENABLE_IPO "Enable Interprocedural Optimization, aka Link Time Optimization (LTO)" ON)

if(ENABLE_IPO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT result OUTPUT output)
    if(result)
        message("IPO enabled: ${output}")
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    else()
        message("IPO is not supported: ${output}")
    endif()
endif()

# Include pthread
#if (WIN32)
#    set (CMAKE_EXE_LINKER_FLAGS "-s -pthread")
#else()
#    set(CMAKE_EXE_LINKER_FLAGS "-s -Wl,--whole-archive -lpthread -Wl,--no-whole-archive -static")
#endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

#add_definitions("-DNDEBUG")
add_definitions("-mavx2")
add_definitions("-funroll-loops")
add_definitions("-fno-rtti")

#add_definitions("-fpermissive")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")

#DBLAZE_USE_CPP_THREADS
#add_definitions("-DBLAZE_USE_CPP_THREADS")
