cmake_minimum_required(VERSION 2.8)

project(server)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include(CTest)

add_executable(main src/main.cpp)

add_library(config STATIC src/config/config.cpp)

target_link_libraries(main config)

enable_testing()

add_subdirectory(tests)