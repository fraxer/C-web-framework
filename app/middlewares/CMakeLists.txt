cmake_minimum_required(VERSION 3.12.4)

FILE(GLOB SOURCES *.c *.h)

set(LIB_NAME middlewares)

add_library(${LIB_NAME} STATIC ${SOURCES})

target_include_directories(${LIB_NAME} PUBLIC .)

target_link_libraries(${LIB_NAME} handler_context protocols)
