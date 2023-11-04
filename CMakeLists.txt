cmake_minimum_required(VERSION 3.12.4)

project(cpdy LANGUAGES C)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/exec")

add_compile_options(-Wall -Wextra -Wpedantic -fPIC)
add_compile_definitions(CMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}")

find_package(Threads REQUIRED)
find_package(PCRE REQUIRED)
find_package(ZLIB REQUIRED)
find_package(OpenSSL)

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

add_subdirectory(apps)
add_subdirectory(libs)
add_subdirectory(handlers)
add_subdirectory(migrations)
add_subdirectory(broadcasting)
