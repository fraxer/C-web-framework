cmake_minimum_required(VERSION 3.12.4)

# The example application.
#
# It builds two ways, from the same files:
#
#   * as part of the monorepo -- backend/CMakeLists.txt does add_subdirectory(app)
#     after add_subdirectory(core), and the framework targets already exist;
#
#   * on its own, against an INSTALLED framework:
#
#       cmake -S backend/app -B build -Dcwfr_DIR=/opt/cwfr/lib/cmake/cwfr
#       cmake --build build && cmake --install build --prefix /srv/myapp
#
#     which is the point of the whole arrangement: the framework is built and
#     installed once, and handlers are written and rebuilt afterwards without
#     the core sources present at all.
#
# The standalone preamble below is the only difference between the two, and it is
# skipped entirely when this directory is not the top of the build.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	project(cwfr_app LANGUAGES C)

	find_package(cwfr 1.0 REQUIRED)

	# The application archives end up inside libapp.so and the handler modules.
	add_compile_options(-fPIC)
	add_link_options(-rdynamic)

	if(CMAKE_BUILD_TYPE STREQUAL Debug OR CMAKE_BUILD_TYPE STREQUAL RelWithDebInfo)
		add_compile_options(-Wall -Wextra -Wpedantic -g)
		add_compile_definitions(DEBUG)
	endif()

	set(CWFR_APP_STANDALONE yes)
endif()

add_subdirectory(models)
add_subdirectory(contexts)
add_subdirectory(middlewares)
add_subdirectory(auth)
add_subdirectory(broadcasting)

# The application as a single shared library.
#
# This is what replaces baking the app archives into libcwfr_framework.so via
# CWFR_EXTRA_FW_LIBS: the framework stays core-only and reusable, while the
# application's stateful code (models, middlewares, contexts) still exists as ONE
# instance that every handler .so resolves at runtime.
#
# cwfr loads it from `main.modules` in config.json and calls app_init().
add_library(app SHARED app_init.c)

target_link_libraries(app PRIVATE
	"-Wl,--whole-archive" models handler_context middlewares auth mybroadcast "-Wl,--no-whole-archive"
	cwfr::framework)

# Handlers include the app headers but must NOT re-link the static archives --
# the code already lives in this .so. Same trick cwfr::framework uses for the core.
foreach(_l models handler_context middlewares auth mybroadcast)
	target_include_directories(app INTERFACE
		$<TARGET_PROPERTY:${_l},INTERFACE_INCLUDE_DIRECTORIES>)
endforeach()

set_target_properties(app PROPERTIES
	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/exec"
	INSTALL_RPATH "$ORIGIN")

# Routes and migrations come last: their handler modules link `app`.
add_subdirectory(routes)
add_subdirectory(migrations)

# In the monorepo the install rules live in backend/CMakeLists.txt, next to the
# framework's. Standalone, this project owns them.
if(CWFR_APP_STANDALONE)
	install(TARGETS app LIBRARY DESTINATION lib/cwfr)
	cwfr_install_handlers()
	cwfr_install_migrations()
endif()
