cmake_minimum_required(VERSION 3.12.4)

file(GLOB FILEPATHS "*.c")

foreach(FILEPATH ${FILEPATHS})
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/exec/handlers")
	get_filename_component(FILENAME ${FILEPATH} NAME_WE)

	add_library(${FILENAME} SHARED ${FILEPATH})

	target_link_libraries(
		${FILENAME}
		base64
		dbquery
		dbresult
		database
		json
		sha1
	)
endforeach()
