cmake_minimum_required(VERSION 3.12.4)

project(cwfr LANGUAGES C)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/core/cmake)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/exec")

# Application-layer build helpers (handlers / migrations). The generic
# cwfr_add_lib and cwfr_add_subdirs helpers live in core/cmake/cwfr.cmake and
# become available once add_subdirectory(core) below is processed.
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/app.cmake)

add_compile_options(-fPIC)
if(CMAKE_BUILD_TYPE STREQUAL Debug OR CMAKE_BUILD_TYPE STREQUAL RelWithDebInfo)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")
	# -fanalyzer is GCC-only and landed in GCC 10; gcc-9 rejects it, which also
	# breaks find_package(Threads) (its try_compile inherits CMAKE_C_FLAGS).
	if(CMAKE_C_COMPILER_ID STREQUAL "GNU" AND CMAKE_C_COMPILER_VERSION VERSION_GREATER_EQUAL 10)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fanalyzer")
	endif()

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
find_package(IDN2 REQUIRED)
find_package(UNISTRING REQUIRED)

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

if(INCLUDE_SQLITE STREQUAL yes)
	find_package(SQLite)
endif()
if(SQLite_FOUND AND INCLUDE_SQLITE STREQUAL yes)
	add_definitions(-DSQLite_FOUND)
endif()

if(BUILD_TESTS STREQUAL yes)
    enable_testing()
endif()

# Application-layer static archives baked into libcwfr_framework.so alongside
# the core (see core/framework_shared/CMakeLists.txt). The core only defines the
# CWFR_EXTRA_FW_LIBS extension point; the concrete app targets are named here,
# where the app layer is known. Targets are defined by the add_subdirectory()
# calls below -- CMake resolves them at generation time. auth and mybroadcast are
# stateless helpers, so handler .so modules link them directly instead of baking
# them in (single shared instance is only needed for stateful registries:
# models, the middlewares_init() hook, and the request/response contexts).
set(CWFR_EXTRA_FW_LIBS models middlewares handler_context)

add_subdirectory(core)
add_subdirectory(app)

# Installing executable files and libraries.
# install(TARGETS) (not install(PROGRAMS)) so INSTALL_RPATH ($ORIGIN) is applied
# at install time -- cwfr/migrate need to find libcwfr_framework.so next to them.
install(TARGETS cwfr migrate RUNTIME DESTINATION bin)

# The shared framework library -- installed alongside handlers/ under lib/cwfr.
# cwfr/migrate reach it via INSTALL_RPATH "$ORIGIN/../lib/cwfr"; handler/migration
# .so modules resolve it from the already-loaded instance (cwfr/migrate load it).
install(TARGETS cwfr_framework LIBRARY DESTINATION lib/cwfr)

# Installing handlers (.so libraries). The trailing slash on the source installs
# the *contents* of CWFR_HANDLER_OUT_DIR into CWFR_HANDLER_INSTALL_DIR, so the
# default "lib/cwfr/handlers" layout is preserved verbatim while an absolute
# -DCWFR_HANDLER_INSTALL_DIR=... redirects handlers independently of --prefix.
install(DIRECTORY ${CWFR_HANDLER_OUT_DIR}/
	DESTINATION ${CWFR_HANDLER_INSTALL_DIR}
	USE_SOURCE_PERMISSIONS
	FILES_MATCHING PATTERN "*.so"
)

# Installing migrations. Same scheme as handlers: contents of
# CWFR_MIGRATION_OUT_DIR into CWFR_MIGRATION_INSTALL_DIR (default lib/cwfr/migrations,
# next to the handlers under lib/cwfr/handlers).
install(DIRECTORY ${CWFR_MIGRATION_OUT_DIR}/
	DESTINATION ${CWFR_MIGRATION_INSTALL_DIR}
	USE_SOURCE_PERMISSIONS
)

include(CPack)
set(CPACK_PACKAGE_NAME "C web framework")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "High-performance C web framework with HTTP/WebSocket support, database integration, and dynamic module loading")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")