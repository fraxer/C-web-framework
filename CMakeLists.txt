cmake_minimum_required(VERSION 2.8)

project(server LANGUAGES C)

#set(CMAKE_CXX_STANDARD 17)
#set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

#find_package(RapidJSON REQUIRED)
find_package(PCRE REQUIRED)

include(CTest)

add_executable(main src/main.c)

add_library(log STATIC src/log/log.c)

add_library(jsmn STATIC src/jsmn/jsmn.c)

add_library(config STATIC src/config/config.c)

add_library(moduleloader STATIC src/moduleloader/moduleloader.c)

add_library(helpers STATIC src/helpers/helpers.c)

add_library(signal STATIC src/signal/signal.c)

add_library(route STATIC src/route/route.c)

add_library(routeloader STATIC src/route/routeloader.c)

add_library(server STATIC src/server/server.c)

add_library(epoll STATIC src/epoll/epoll.c)

add_library(domain STATIC src/domain/domain.c)

add_library(socket STATIC src/socket/socket.c)

#add_library(database STATIC src/database/database.cpp)

#add_library(openssl STATIC src/openssl/openssl.cpp)

#add_library(mimetype STATIC src/mimetype/mimetype.cpp)

add_library(thread STATIC src/thread/thread.c)

# Pages
add_library(indexpage SHARED src/handlers/index.c)



target_link_libraries(
	config
	jsmn
	log
	helpers
)

target_link_libraries(
	routeloader
	${CMAKE_DL_LIBS}
)

target_link_libraries(
	moduleloader
	config
	route
	routeloader
#	database
#	openssl
#	mimetype
	thread
	domain
	server
	epoll
	socket

	pthread
	pcre
)

target_link_libraries(
	main
	moduleloader
	signal
)

#enable_testing()

#add_subdirectory(tests)