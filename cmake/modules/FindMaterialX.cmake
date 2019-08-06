#
# Copyright 2018 Pixar
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
# - Find MaterialX Development Kit
#
# Finds an installed MaterialX Development Kit
#
# Variables that will be defined:
# MATERIALX_FOUND            Defined if MaterialX has been detected
# MATERIALX_BASE_DIR         Path to the root of the MaterialX installation 
# MATERIALX_INCLUDE_DIRS     Path to the MaterialX include directories
# MATERIALX_LIB_DIRS         Path to the MaterialX libraray directories
# MATERIALX_STDLIB_DIR       Path to the MaterialX standard library directory
# MATERIALX_LIBRARIES        List of MaterialX libraries

#
# In:
#  MATERIALX_ROOT
#
# Out:
#  MATERIALX_FOUND
#  MATERIALX_INCLUDE_DIRS
#  MATERIALX_LIB_DIRS
#  MATERIALX_LIBRARIES

find_path(MATERIALX_BASE_DIR
    NAMES
        include/MaterialXCore/Library.h
    HINTS
        "${MATERIALX_ROOT}"
        "$ENV{MATERIALX_ROOT}"
    )

find_path(MATERIALX_INCLUDE_DIRS 
    MaterialXCore/Library.h
    HINTS
        "${MATERIALX_ROOT}"
        "$ENV{MATERIALX_ROOT}"        
        "${MATERIALX_BASE_DIR}"
    PATH_SUFFIXES
        include
    DOC
        "MaterialX Header Path"
)

if (WIN32)
    set(MATERIALX_CORE_LIB_NAME MaterialXCore.lib)
else()
    set(MATERIALX_CORE_LIB_NAME libMaterialXCore.a)
endif()

find_path(MATERIALX_LIB_DIRS 
    "${MATERIALX_CORE_LIB_NAME}"
    HINTS
        "${MATERIALX_ROOT}"
        "$ENV{MATERIALX_ROOT}"        
        "${MATERIALX_BASE_DIR}"
    PATH_SUFFIXES
        lib
    DOC
        "MaterialX Library Path"
)

find_path(MATERIALX_STDLIB_DIR 
    stdlib_defs.mtlx
    HINTS
        "${MATERIALX_ROOT}"
        "$ENV{MATERIALX_ROOT}"        
        "${MATERIALX_BASE_DIR}"
    PATH_SUFFIXES
        documents/Libraries
    DOC
        "MaterialX Standard Libraries Path"
)

foreach(MATERIALX_LIB
    Core
    Format)
    find_library(MATERIALX_${MATERIALX_LIB}_LIBRARY
            MaterialX${MATERIALX_LIB}
        HINTS
            "${MATERIALX_LIB_DIRS}"
        DOC
            "MaterialX's ${MATERIALX_LIB} library path"
        NO_CMAKE_SYSTEM_PATH
    )

    if (MATERIALX_${MATERIALX_LIB}_LIBRARY)
        list(APPEND MATERIALX_LIBRARIES ${MATERIALX_${MATERIALX_LIB}_LIBRARY})
    endif ()
endforeach()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MaterialX
    REQUIRED_VARS
        MATERIALX_BASE_DIR
        MATERIALX_INCLUDE_DIRS
        MATERIALX_LIB_DIRS
        MATERIALX_STDLIB_DIR
)
