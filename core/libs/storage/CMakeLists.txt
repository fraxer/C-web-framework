cmake_minimum_required(VERSION 3.12.4)

FILE(GLOB SOURCES *.c *.h)

set(LIB_NAME storage)

add_library(${LIB_NAME} STATIC ${SOURCES})

target_include_directories(${LIB_NAME} PUBLIC . ${LIBXML2_INCLUDE_DIR})

target_link_libraries(${LIB_NAME} client config log misc ${LIBXML2_LIBRARIES})
