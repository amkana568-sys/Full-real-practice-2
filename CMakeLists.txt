cmake_minimum_required(VERSION 3.16)
project(DependencyVisualizerV23 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(depviz
    src/config.cpp
    src/repository.cpp
    src/graph.cpp
)
target_include_directories(depviz PUBLIC include)

if(MSVC)
    target_compile_options(depviz PRIVATE /W4)
else()
    target_compile_options(depviz PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(visualizer src/main.cpp)
target_link_libraries(visualizer PRIVATE depviz)
