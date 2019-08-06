#
# Copyright 2019 Pixar
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

#

if(UNIX)
    find_path(OCIO_BASE_DIR
            include/OpenColorIO/OpenColorABI.h
        HINTS
            "${OCIO_LOCATION}"
            "$ENV{OCIO_LOCATION}"
            "/opt/ocio"
    )
    find_path(OCIO_LIBRARY_DIR
            libOpenColorIO.so
        HINTS
            "${OCIO_LOCATION}"
            "$ENV{OCIO_LOCATION}"
            "${OCIO_BASE_DIR}"
        PATH_SUFFIXES
            lib/
        DOC
            "OpenColorIO library path"
    )
elseif(WIN32)
    find_path(OCIO_BASE_DIR
            include/OpenColorIO/OpenColorABI.h
        HINTS
            "${OCIO_LOCATION}"
            "$ENV{OCIO_LOCATION}"
    )
    find_path(OCIO_LIBRARY_DIR
            OpenColorIO.lib
        HINTS
            "${OCIO_LOCATION}"
            "$ENV{OCIO_LOCATION}"
            "${OCIO_BASE_DIR}"
        PATH_SUFFIXES
            lib/
        DOC
            "OpenColorIO library path"
    )
endif()

find_path(OCIO_INCLUDE_DIR
        OpenColorIO/OpenColorABI.h
    HINTS
        "${OCIO_LOCATION}"
        "$ENV{OCIO_LOCATION}"
        "${OCIO_BASE_DIR}"
    PATH_SUFFIXES
        include/
    DOC
        "OpenColorIO headers path"
)

list(APPEND OCIO_INCLUDE_DIRS ${OCIO_INCLUDE_DIR})

find_library(OCIO_LIBRARY
        OpenColorIO
    HINTS
        "${OCIO_LOCATION}"
        "$ENV{OCIO_LOCATION}"
        "${OCIO_BASE_DIR}"
    PATH_SUFFIXES
        lib/
    DOC
        "OCIO's ${OCIO_LIB} library path"
)

list(APPEND OCIO_LIBRARIES ${OCIO_LIBRARY})

if(OCIO_INCLUDE_DIRS AND EXISTS "${OCIO_INCLUDE_DIR}/OpenColorIO/OpenColorABI.h")
    file(STRINGS ${OCIO_INCLUDE_DIR}/OpenColorIO/OpenColorABI.h
        fullVersion
        REGEX
        "#define OCIO_VERSION .*$")
    string(REGEX MATCH "[0-9]+.[0-9]+.[0-9]+" OCIO_VERSION ${fullVersion})
endif()

# handle the QUIETLY and REQUIRED arguments and set OCIO_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(OpenColorIO
    REQUIRED_VARS
        OCIO_LIBRARIES
        OCIO_INCLUDE_DIRS
    VERSION_VAR
        OCIO_VERSION
)
