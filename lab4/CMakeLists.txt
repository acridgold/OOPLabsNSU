cmake_minimum_required(VERSION 3.15)
project(lab4)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP ON
)
FetchContent_MakeAvailable(googletest)

add_executable(lab4
        src/main.cpp
)

add_executable(test_lab4
        src/test_main.cpp
        src/test_main.h
)

target_link_libraries(test_lab4 PRIVATE gtest_main)
target_include_directories(test_lab4 PRIVATE .)

include(GoogleTest)
gtest_discover_tests(test_lab4)

if(CMAKE_BUILD_TYPE STREQUAL "Coverage")
    target_compile_options(lab4 PRIVATE -fprofile-arcs -ftest-coverage)
    target_link_options(lab4 PRIVATE -fprofile-arcs -ftest-coverage)
endif()

option(ENABLE_PROFILING "Enable profiling with gprof" OFF)
if(ENABLE_PROFILING)
    target_compile_options(lab4 PRIVATE -pg)
    target_link_options(lab4 PRIVATE -pg)
endif()

if(MINGW)
    target_link_options(lab4 PRIVATE -static -static-libgcc -static-libstdc++)
endif()

if(APPLE)
    target_link_libraries(lab4 PRIVATE -framework IOKit -framework Cocoa -framework OpenGL)
endif()
