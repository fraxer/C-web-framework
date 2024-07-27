cmake_minimum_required(VERSION 3.12.4)

set(LIB_NAME database)

if(INCLUDE_MYSQL STREQUAL yes)
	#sudo apt install libmysqlclient-dev 
	message(STATUS "Include Mysql: ${INCLUDE_MYSQL}")
endif()

if(MySQL_FOUND AND INCLUDE_MYSQL STREQUAL yes)
	include_directories(${MySQL_INCLUDE_DIRS})
endif()

if(INCLUDE_POSTGRESQL STREQUAL yes)
	message(STATUS "Include Postgresql: ${INCLUDE_POSTGRESQL}")
endif()

if(PostgreSQL_FOUND AND INCLUDE_POSTGRESQL STREQUAL yes)
	include_directories(${PostgreSQL_INCLUDE_DIRS} ${PostgreSQL_SERVER_INCLUDE_DIRS})
endif()

if(INCLUDE_REDIS STREQUAL yes)
	#sudo apt-get install -y libhiredis-dev
	message(STATUS "Include Redis: ${INCLUDE_REDIS}")
endif()

if(Redis_FOUND AND INCLUDE_REDIS STREQUAL yes)
	include_directories(${REDIS_INCLUDE_DIRS})
endif()


set(SOURCES "")
set(LIBS "")

list(APPEND LIBS misc)
list(APPEND LIBS log)
list(APPEND LIBS json)
list(APPEND LIBS config)

list(APPEND SOURCES database.c database.h)
list(APPEND SOURCES dbquery.c dbquery.h)
list(APPEND SOURCES dbresult.c dbresult.h)
list(APPEND SOURCES db.h)

if(MySQL_FOUND AND INCLUDE_MYSQL STREQUAL yes)
	list(APPEND SOURCES mysql.c mysql.h)
	list(APPEND LIBS ${MySQL_LIBRARY})
endif()

if(PostgreSQL_FOUND AND INCLUDE_POSTGRESQL STREQUAL yes)
	list(APPEND SOURCES postgresql.c postgresql.h)
	list(APPEND LIBS pq)
endif()

if(Redis_FOUND AND INCLUDE_REDIS STREQUAL yes)
	list(APPEND SOURCES redis.c redis.h)
	list(APPEND LIBS ${Redis_LIBRARY})
endif()

add_library(${LIB_NAME} STATIC ${SOURCES})

target_include_directories(${LIB_NAME} PUBLIC .)

target_link_libraries(${LIB_NAME} ${LIBS})
