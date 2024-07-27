cmake_minimum_required(VERSION 3.12.4)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,--exclude-libs,libpcre")

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
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/exec/migrations/${DIR}")

	file(GLOB FILEPATHS "${DIR}/*.c")

	foreach(FILEPATH ${FILEPATHS})
		get_filename_component(FILENAME ${FILEPATH} NAME_WE)

		add_library(${FILENAME} SHARED ${FILEPATH})
		target_link_libraries(${FILENAME} database)
	endforeach()
endforeach()
