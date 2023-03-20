#-- Installing: /usr/local/lib/liberedis.so
# Try to find Redis headers and library.
#
# Usage of this module as follows:
#
#     find_package(Redis)
#
# Variables used by this module, they can change the default behaviour and need
# to be set before calling find_package:
#
#  Redis_ROOT_DIR          Set this variable to the root installation of
#                            Redis if the module has problems finding the
#                            proper installation path.
#
# Variables defined by this module:
#
#  Redis_FOUND               System has Redis library/headers.
#  Redis_LIBRARIES           The Redis library.
#  Redis_INCLUDE_DIRS        The location of Redis headers.

find_path(Redis_DIR
    HINTS
    /usr
    /usr/local
)

find_path(Redis_INCLUDE_DIR hiredis/hiredis.h
    HINTS
    /usr
    /usr/local
    ${Redis_DIR}
    PATH_SUFFIXES include
)

find_library(Redis_LIBRARY hiredis
    HINTS
    /usr
    /usr/lib/
    ${Redis_DIR}
    PATH SUFFIXES lib
)

set(Redis_INCLUDE_DIRS ${Redis_INCLUDE_DIR})
set(Redis_LIBRARIES ${Redis_LIBRARY})


include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Redis DEFAULT_MSG Redis_LIBRARY Redis_INCLUDE_DIR)

if(Redis_FOUND)
  message(STATUS "Redis found")
endif()

mark_as_advanced(
    Redis_DIR
    Redis_LIBRARY
    Redis_INCLUDE_DIR
)
