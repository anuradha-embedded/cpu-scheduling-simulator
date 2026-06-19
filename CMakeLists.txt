cmake_minimum_required(VERSION 3.14)

project(cpu_scheduling_simulator
        VERSION 1.0.0
        DESCRIPTION "Simulate and compare classic CPU scheduling algorithms"
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    add_compile_options(-Wall -Wextra)
endif()

# Core scheduling library, shared by the CLI and the test binary.
add_library(cpusim_core
    src/process.cpp
    src/scheduler.cpp
    src/report.cpp)
target_include_directories(cpusim_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Main command-line tool.
add_executable(cpusim src/main.cpp)
target_link_libraries(cpusim PRIVATE cpusim_core)

# Tests.
enable_testing()
add_executable(test_scheduler tests/test_scheduler.cpp)
target_link_libraries(test_scheduler PRIVATE cpusim_core)
add_test(NAME scheduler_tests COMMAND test_scheduler)
