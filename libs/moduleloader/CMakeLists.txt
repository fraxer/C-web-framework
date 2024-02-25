cmake_minimum_required(VERSION 3.12.4)

FILE(GLOB SOURCES *.c *.h)

set(LIB_NAME moduleloader)

add_library(${LIB_NAME} STATIC ${SOURCES})

target_include_directories(${LIB_NAME} PUBLIC .)

target_link_libraries(${LIB_NAME} storage client config log redirect route domain server openssl mimetype thread json database broadcast ${CMAKE_DL_LIBS})
