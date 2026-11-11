cmake_minimum_required(VERSION 3.16)
project(DependencyVisualizerV23 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(visualizer src/main.cpp src/config.cpp)
target_include_directories(visualizer PRIVATE include)

if(MSVC)
    target_compile_options(visualizer PRIVATE /W4)
else()
    target_compile_options(visualizer PRIVATE -Wall -Wextra -Wpedantic)
endif()
