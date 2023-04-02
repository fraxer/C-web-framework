cmake_minimum_required(VERSION 2.8)

project(migrations LANGUAGES C)

MACRO(SUBDIRLIST result curdir)
  FILE(GLOB children RELATIVE ${curdir} ${curdir}/*)
  SET(dirlist "")
  FOREACH(child ${children})
    IF(IS_DIRECTORY ${curdir}/${child})
      LIST(APPEND dirlist ${child})
    ENDIF()
  ENDFOREACH()
  SET(${result} ${dirlist})
ENDMACRO()

SUBDIRLIST(SUBDIRS ${CMAKE_CURRENT_SOURCE_DIR})

foreach(DIR ${SUBDIRS})
    message(STATUS ${DIR})
endforeach()

file(GLOB FILEPATHS "src/*.c")

foreach(FILEPATH ${FILEPATHS})
    get_filename_component(FILENAME ${FILEPATH} NAME_WE)
    message(STATUS "${FILENAME}")

    add_library(${FILENAME} SHARED ${FILEPATH})
endforeach()

add_executable(migrate main.c)