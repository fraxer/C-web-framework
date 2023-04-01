cmake_minimum_required(VERSION 2.8)

project(migration LANGUAGES C)

find_package(OpenSSL)

if(INCLUDE_MYSQL STREQUAL yes)
	#sudo apt install libmysqlclient-dev 
	find_package(MySQL)
	message(STATUS "Include Mysql: ${INCLUDE_MYSQL}")
endif()

if(MySQL_FOUND AND INCLUDE_MYSQL STREQUAL yes)
	include_directories(${MySQL_INCLUDE_DIRS})
	add_definitions(-DMySQL_FOUND)
endif()

if(INCLUDE_POSTGRESQL STREQUAL yes)
	find_package(PostgreSQL)
	message(STATUS "Include Postgresql: ${INCLUDE_POSTGRESQL}")
endif()

if(PostgreSQL_FOUND AND INCLUDE_POSTGRESQL STREQUAL yes)
	include_directories(${PostgreSQL_INCLUDE_DIRS} ${PostgreSQL_SERVER_INCLUDE_DIRS})
	add_definitions(-DPostgreSQL_FOUND)
endif()

if(INCLUDE_REDIS STREQUAL yes)
	#sudo apt-get install -y libhiredis-dev
	find_package(Redis)
	message(STATUS "Include Redis: ${INCLUDE_REDIS}")
endif()

if(Redis_FOUND AND INCLUDE_REDIS STREQUAL yes)
	include_directories(${REDIS_INCLUDE_DIRS})
	add_definitions(-DRedis_FOUND)
endif()

add_executable(migration main.c)