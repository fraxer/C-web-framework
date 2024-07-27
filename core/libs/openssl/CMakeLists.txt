cmake_minimum_required(VERSION 3.12.4)

FILE(GLOB SOURCES *.c *.h)

set(LIB_NAME openssl)

add_library(${LIB_NAME} STATIC ${SOURCES})

target_include_directories(${LIB_NAME} PUBLIC .)

target_link_libraries(${LIB_NAME} log ${OPENSSL_LIBRARIES})
