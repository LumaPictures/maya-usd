#
# Simple module to find UFE.
#
# This module searches for a valid UFE installation.
# It searches for UFE's libraries and include header files.
#
# Variables that will be defined:
# UFE_FOUND           Defined if a UFE installation has been detected
# UFE_LIBRARY         Path to UFE library
# UFE_INCLUDE_DIR     Path to the UFE include directory
#

find_path(UFE_INCLUDE_DIR
        ufe/versionInfo.h
    HINTS
        ${UFE_INCLUDE_ROOT}
        ${MAYA_DEVKIT_LOCATION}
        ${MAYA_LOCATION}
        $ENV{MAYA_LOCATION}
        ${MAYA_BASE_DIR}
    PATH_SUFFIXES
        devkit/ufe/include
        include/
    DOC
        "UFE header path"
)

find_library(UFE_LIBRARY
    NAMES
        ufe_${UFE_MAJOR_VERSION}
    HINTS
        ${UFE_LIB_ROOT}
        ${MAYA_DEVKIT_LOCATION}
        ${MAYA_LOCATION}
        $ENV{MAYA_LOCATION}
        ${MAYA_BASE_DIR}
    PATHS
        ${UFE_LIBRARY_DIR}
    PATH_SUFFIXES
        devkit/ufe/lib
        lib/
    DOC
        "UFE library"
    NO_DEFAULT_PATH
)

message(STATUS "UFE Include directory: ${UFE_INCLUDE_DIR}")
message(STATUS "         UFE Library : ${UFE_LIBRARY}")

# Handle the QUIETLY and REQUIRED arguments and set UFE_FOUND to TRUE if
# all listed variables are TRUE.
include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(UFE
    REQUIRED_VARS
        UFE_INCLUDE_DIR
        UFE_LIBRARY
)
