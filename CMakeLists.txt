cmake_minimum_required(VERSION 2.8)

project(server)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(RapidJSON REQUIRED)

include(CTest)

add_executable(main src/main.cpp)

add_library(log STATIC src/log/log.cpp)

add_library(config STATIC src/config/config.cpp)

add_library(moduleLoader STATIC src/moduleLoader/moduleLoader.cpp)

add_library(helpers STATIC src/helpers/helpers.cpp)

add_library(signal STATIC src/signal/signal.cpp)

add_library(route STATIC src/route/route.cpp)

add_library(database STATIC src/database/database.cpp)

add_library(openssl STATIC src/openssl/openssl.cpp)

add_library(mimetype STATIC src/mimetype/mimetype.cpp)

add_library(thread STATIC src/thread/thread.cpp)

target_link_libraries(
	config
	log
	helpers
)

target_link_libraries(
	moduleLoader
	config
	route
	database
	openssl
	mimetype
	thread
)

target_link_libraries(
	main
	moduleLoader
	signal
)

enable_testing()

add_subdirectory(tests)