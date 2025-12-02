cmake_minimum_required(VERSION 3.12.4)

project(cpdy LANGUAGES C)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/core/cmake)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/exec")

add_compile_options(-fPIC)
if(CMAKE_BUILD_TYPE STREQUAL Debug OR CMAKE_BUILD_TYPE STREQUAL RelWithDebInfo)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fanalyzer")

	add_compile_options(-Wall -Wextra -Wpedantic -fsanitize=address -fsanitize=leak -g)
	add_compile_definitions(DEBUG)
endif()

add_link_options(-rdynamic)
add_compile_definitions(CMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}")

find_package(Threads REQUIRED)
find_package(PCRE REQUIRED)
find_package(ZLIB REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(LibXml2 REQUIRED)

if(INCLUDE_MYSQL STREQUAL yes)
	find_package(MySQL)
endif()
if(MySQL_FOUND AND INCLUDE_MYSQL STREQUAL yes)
	add_definitions(-DMySQL_FOUND)
endif()

if(INCLUDE_POSTGRESQL STREQUAL yes)
	find_package(PostgreSQL)
endif()
if(PostgreSQL_FOUND AND INCLUDE_POSTGRESQL STREQUAL yes)
	add_definitions(-DPostgreSQL_FOUND)
endif()

if(INCLUDE_REDIS STREQUAL yes)
	find_package(Redis)
endif()
if(Redis_FOUND AND INCLUDE_REDIS STREQUAL yes)
	add_definitions(-DRedis_FOUND)
endif()

if(BUILD_TESTS STREQUAL yes)
    enable_testing()
endif()

add_subdirectory(core)
add_subdirectory(app)

# Installing executable files and libraries
install(PROGRAMS
	${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/cpdy
	${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/migrate
	DESTINATION bin
)

# Installing handlers (.so libraries)
install(DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/handlers
	DESTINATION lib/cpdy
	USE_SOURCE_PERMISSIONS
	FILES_MATCHING PATTERN "*.so"
)

# Installing migrations
install(DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/migrations
	DESTINATION share/cpdy
	USE_SOURCE_PERMISSIONS
)

include(CPack)
set(CPACK_PACKAGE_NAME "C web framework")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "High-performance C web framework with HTTP/WebSocket support, database integration, and dynamic module loading")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")