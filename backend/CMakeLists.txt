cmake_minimum_required(VERSION 3.12.4)

project(cwfr LANGUAGES C)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/core/cmake)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/exec")

# The build helpers (cwfr_add_lib, cwfr_add_handlers, cwfr_add_migrations, the
# cwfr_install_* rules) come from core/cmake and are included by add_subdirectory(core)
# below. An application built against an INSTALLED framework gets the same
# helpers from find_package(cwfr) -- see app-standalone/CMakeLists.txt.

add_compile_options(-fPIC)

# Sanitizer selection. ASan+LSan stay the default for the debug build types, so
# an unqualified `cmake .. -DCMAKE_BUILD_TYPE=Debug` behaves exactly as before.
# TSan is opt-in (-DSANITIZE=thread) because it cannot be linked alongside ASan,
# and it is the tool for the connection-level data races (docs/concurrency/00).
set(SANITIZE "default" CACHE STRING "Sanitizer: default|address|thread|none")
set_property(CACHE SANITIZE PROPERTY STRINGS default address thread none)

if(SANITIZE STREQUAL default)
	if(CMAKE_BUILD_TYPE STREQUAL Debug OR CMAKE_BUILD_TYPE STREQUAL RelWithDebInfo)
		set(SANITIZE address)
	else()
		set(SANITIZE none)
	endif()
endif()

# Flags go into CMAKE_C_FLAGS rather than add_compile_options: the sanitizer
# runtime has to reach the link line too, and CMAKE_C_FLAGS is what try_compile
# and the link rules both inherit.
if(SANITIZE STREQUAL address)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fsanitize=leak -g")
elseif(SANITIZE STREQUAL thread)
	# -O1 keeps the instrumented server fast enough to actually reach the
	# concurrency it is meant to exercise; at -O0 TSan slows things down until
	# the interesting interleavings stop happening.
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=thread -O1 -g")
	add_compile_definitions(CWFR_TSAN)
elseif(NOT SANITIZE STREQUAL none)
	message(FATAL_ERROR "SANITIZE must be one of: default, address, thread, none (got '${SANITIZE}')")
endif()

# Coverage instrumentation for fuzzing (docs/http3/08-testing.md §5).
#
# Global, and it has to be: the fuzzer's feedback comes from edges inside the
# *parsers*, which live in the framework, not in the target files. Behind its
# own flag so an ordinary build never carries it.
#
# gcc has no libFuzzer, but it does have the signal libFuzzer feeds on:
# -fsanitize-coverage=trace-pc calls __sanitizer_cov_trace_pc() on every edge,
# and tests/fuzz/fuzz_main.c supplies the loop around it. With clang the flag is
# libFuzzer's own and the driver is not compiled at all.
set(BUILD_FUZZERS "no" CACHE STRING "Build fuzz targets: yes|no")

# add_compile_options, NOT CMAKE_C_FLAGS: the latter is inherited by every
# try_compile, and a probe program that calls __sanitizer_cov_trace_pc without
# defining it fails to link -- which surfaces as `find_package(Threads)` deciding
# this compiler does not support -pthread. (The -fanalyzer comment below is the
# same trap, found the same way.)
if(BUILD_FUZZERS STREQUAL yes)
	if(CMAKE_C_COMPILER_ID MATCHES "Clang")
		add_compile_options(-fsanitize=fuzzer-no-link)
	else()
		add_compile_options(-fsanitize-coverage=trace-pc)
		# The framework then calls an edge hook that only a fuzzer defines, and
		# it is a shared library loaded by the server and every handler module.
		# core/misc/fuzzcov.c carries a weak no-op for them; the driver's own
		# definition preempts it in the fuzz binaries.
		add_compile_definitions(CWFR_FUZZ_COVERAGE)
	endif()
endif()

if(CMAKE_BUILD_TYPE STREQUAL Debug OR CMAKE_BUILD_TYPE STREQUAL RelWithDebInfo)
	# -fanalyzer is GCC-only and landed in GCC 10; gcc-9 rejects it, which also
	# breaks find_package(Threads) (its try_compile inherits CMAKE_C_FLAGS).
	if(CMAKE_C_COMPILER_ID STREQUAL "GNU" AND CMAKE_C_COMPILER_VERSION VERSION_GREATER_EQUAL 10)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fanalyzer")
	endif()

	add_compile_options(-Wall -Wextra -Wpedantic -g)
	add_compile_definitions(DEBUG)
endif()

add_link_options(-rdynamic)

find_package(Threads REQUIRED)
find_package(PCRE2 REQUIRED)
add_definitions(-DPCRE2_CODE_UNIT_WIDTH=8)
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

# Nothing from the application is baked into libcwfr_framework.so any more: the
# framework binary is core-only and can be built and installed once, independently
# of this (or any other) application. CWFR_EXTRA_FW_LIBS still exists as an
# extension point in core/framework_shared for applications that have not been
# migrated, and is deliberately left empty here.
#
# The application ships as its own shared library, libapp.so, listed under
# `main.modules` in config.json. cwfr dlopens it before parsing `servers` and
# calls app_init(), which registers the middlewares and the context destructors
# the core used to resolve at link time.
add_subdirectory(core)
add_subdirectory(app)

# The framework installs itself: cwfr, migrate, libcwfr_framework.so, the public
# headers and the CMake package all come from core/cmake/install.cmake. What is
# left here is the application's own output.
#
# libapp.so goes next to the framework library, because that is what the
# `main.modules` path in an installed config.json points at.
install(TARGETS app LIBRARY DESTINATION lib/cwfr)

cwfr_install_handlers()
cwfr_install_migrations()


include(CPack)
set(CPACK_PACKAGE_NAME "C web framework")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "High-performance C web framework with HTTP/WebSocket support, database integration, and dynamic module loading")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")