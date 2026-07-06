# Shared CMake helpers for the application layer (backend/app).
#
# These collapse the per-file boilerplate that was duplicated verbatim in the
# old app/routes and app/migrations CMakeLists.txt: one SHARED handler (.so) per
# *.c file under routes/, and one SHARED migration (.so) per *.c file under each
# migrations/<server> subdirectory. The generic static-library and subdirectory
# helpers live in core/cmake/cwfr.cmake (cwfr_add_lib / cwfr_add_subdirs) and are
# reused by the app static archives (auth / models / middlewares / ...).
#
# Unlike the multi-service services.cmake this single-app build has no per-module
# namespace and no gettext locales, so handler/migration targets keep their
# historical names and output paths -- config.json references them as
# handlers/<sub>/lib_<name>.so and migrations/<server>/lib<name>.so verbatim.

# Configurable output directories for handler/migration .so modules. Override at
# configure time (-DCWFR_HANDLER_OUT_DIR=..., -DCWFR_MIGRATION_OUT_DIR=...) to
# emit them outside the build tree (e.g. into $HOME) so they can be rebuilt
# independently of an installed binary prefix. Default to the build tree to
# preserve the historical layout and the install() rules in backend/CMakeLists.txt.
set(CWFR_HANDLER_OUT_DIR   "${CMAKE_BINARY_DIR}/exec/handlers"
	CACHE PATH "Where handler .so modules are written (module/sub-path appended)")
set(CWFR_MIGRATION_OUT_DIR "${CMAKE_BINARY_DIR}/exec/migrations"
	CACHE PATH "Where migration .so modules are written (sub-dir appended)")

# Configurable INSTALL destinations for those same modules. Override at configure
# time (-DCWFR_HANDLER_INSTALL_DIR=..., -DCWFR_MIGRATION_INSTALL_DIR=...) to send
# handlers/migrations to a different root than the binaries at `cmake --install`
# time. A relative path is resolved under CMAKE_INSTALL_PREFIX (the default); an
# ABSOLUTE path is used verbatim and ignores --prefix -- handy for keeping
# frequently-rebuilt modules in $HOME separate from a stable binary install.
set(CWFR_HANDLER_INSTALL_DIR   "lib/cwfr/handlers"
	CACHE PATH "Install destination for handler .so modules (relative => under prefix; absolute => verbatim)")
set(CWFR_MIGRATION_INSTALL_DIR "lib/cwfr/migrations"
	CACHE PATH "Install destination for migration .so modules (relative => under prefix; absolute => verbatim)")

# Build one SHARED handler (.so) per *.c file under routes/, named _<file> and
# written to exec/handlers/<relative-path>/lib_<file>.so. Each handler excludes
# libpcre symbols from its export table (previously -Wl,--exclude-libs,libpcre
# injected via CMAKE_C_FLAGS; now a per-target link flag, which is where it
# belongs). The framework (and the app archives baked into it via
# CWFR_EXTRA_FW_LIBS) is resolved at runtime from the single libcwfr_framework.so
# instance that cwfr/migrate load, so handler modules stay small.
function(cwfr_add_handlers)
	cmake_parse_arguments(ARG "" "" "INCLUDE_DIRS;LINK_LIBS" ${ARGN})

	file(GLOB_RECURSE _filepaths *.c)

	foreach(_filepath ${_filepaths})
		get_filename_component(_filedir ${_filepath} DIRECTORY)
		get_filename_component(_filename ${_filepath} NAME_WE)
		file(RELATIVE_PATH _rel ${CMAKE_CURRENT_SOURCE_DIR} ${_filedir})

		set(_target _${_filename})
		set(_out_dir "${CWFR_HANDLER_OUT_DIR}/${_rel}")

		add_library(${_target} SHARED ${_filepath})
		set_target_properties(${_target} PROPERTIES
			LIBRARY_OUTPUT_DIRECTORY ${_out_dir}
			LINK_FLAGS "-Wl,--exclude-libs,libpcre")

		if(ARG_INCLUDE_DIRS)
			target_include_directories(${_target} PRIVATE ${ARG_INCLUDE_DIRS})
		endif()
		target_link_libraries(${_target} ${ARG_LINK_LIBS})
	endforeach()
endfunction()

# Build one SHARED migration (.so) per *.c file in each subdirectory of the
# current directory (s1/, s2/, ...). Targets are <file>, output to
# exec/migrations/<subdir>/. The framework is resolved at runtime from the
# already-loaded libcwfr_framework.so, so migration modules stay small.
function(cwfr_add_migrations)
	file(GLOB _children RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*)
	foreach(_subdir ${_children})
		if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${_subdir})
			file(GLOB _filepaths "${_subdir}/*.c")
			set(_out_dir "${CWFR_MIGRATION_OUT_DIR}/${_subdir}")

			foreach(_filepath ${_filepaths})
				get_filename_component(_filename ${_filepath} NAME_WE)

				add_library(${_filename} SHARED ${_filepath})
				set_target_properties(${_filename} PROPERTIES
					LIBRARY_OUTPUT_DIRECTORY ${_out_dir})
				target_link_libraries(${_filename} cwfr_framework)
			endforeach()
		endif()
	endforeach()
endfunction()
