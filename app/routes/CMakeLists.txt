cmake_minimum_required(VERSION 3.12.4)

file(GLOB_RECURSE FILEPATHS *.c)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,--exclude-libs,libpcre")

foreach(FILEPATH ${FILEPATHS})
    get_filename_component(FILEDIR ${FILEPATH} DIRECTORY)
    get_filename_component(FILENAME ${FILEPATH} NAME_WE)

	file(RELATIVE_PATH RELATIVE_PATH_VARIABLE ${CMAKE_CURRENT_SOURCE_DIR} ${FILEDIR})

    set(TARGET_NAME ${FILENAME})
    set(TARGET_SOURCES ${FILEPATH})

    set(TARGET_OUTPUT_DIR "${CMAKE_BINARY_DIR}/exec/handlers/${RELATIVE_PATH_VARIABLE}")

    add_library(${TARGET_NAME} SHARED ${TARGET_SOURCES})
    set_target_properties(${TARGET_NAME} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${TARGET_OUTPUT_DIR}
    )

    target_link_libraries(${TARGET_NAME} models view middlewares handler_context mail storage misc database json request response protocols broadcast client mybroadcast)
endforeach()
