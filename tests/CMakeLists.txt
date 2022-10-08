cmake_minimum_required(VERSION 2.8)

add_library(common STATIC common.cpp)

# def macros CMAKE_SOURCE_DIR
target_compile_definitions(common PRIVATE CMAKE_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

add_subdirectory(config)
add_subdirectory(helpers)
