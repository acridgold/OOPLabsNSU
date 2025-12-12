cmake_minimum_required(VERSION 3.15)
project(lab3)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(lab3
        src/main.cpp
        src/ConfigParser.cpp
        src/Converter.cpp
        src/Converter.h
        src/ConfigParser.h
        src/WavFile.cpp
        src/WavFile.h
)

# Для покрытия кода
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(lab3 PRIVATE --coverage)
    target_link_options(lab3 PRIVATE --coverage)
endif ()

option(ENABLE_PROFILING "Enable profiling with gprof" OFF)
if (ENABLE_PROFILING)
    target_compile_options(lab3 PRIVATE -pg)
    target_link_options(lab3 PRIVATE -pg)
    message(STATUS "Profiling enabled with -pg flag")
endif ()

if (MINGW)
    target_link_options(lab3 PRIVATE "-static" "-static-libgcc" "-static-libstdc++")
endif ()

if (APPLE)
    target_link_libraries(lab3 PRIVATE "-framework IOKit")
    target_link_libraries(lab3 PRIVATE "-framework Cocoa")
    target_link_libraries(lab3 PRIVATE "-framework OpenGL")
endif ()