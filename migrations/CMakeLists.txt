cmake_minimum_required(VERSION 2.8)

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
	endforeach()
endforeach()

add_executable(migrate main.c)

target_link_libraries(migrate jsmn config moduleloader database ${CMAKE_DL_LIBS})

if(MySQL_FOUND AND INCLUDE_MYSQL STREQUAL yes)
	target_link_libraries(
		migrate
		mysql
		${MySQL_LIBRARY}
	)
endif()

if(PostgreSQL_FOUND AND INCLUDE_POSTGRESQL STREQUAL yes)
	target_link_libraries(
		migrate
		postgresql
		pq
	)
endif()

if(Redis_FOUND AND INCLUDE_REDIS STREQUAL yes)
	target_link_libraries(
		migrate
		redis
		${Redis_LIBRARY}
	)
endif()

