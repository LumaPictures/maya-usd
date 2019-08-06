#
# Copyright 2016 Pixar
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
if (NOT PYTHON_EXECUTABLE)
    message(FATAL_ERROR "Unable to find Python executable - PySide not present")
    return()
endif()

execute_process(
    COMMAND "${PYTHON_EXECUTABLE}" "-c" "import PySide"
    RESULT_VARIABLE pySideImportResult 
)

# Pyside is preferred over PySide2 since PySide2 support is still experimental.
# If this changes, be sure that the Usdviewq.qt module has the same behavior.

if (pySideImportResult EQUAL 1 OR PYSIDE_USE_PYSIDE2)
    execute_process(
        COMMAND "${PYTHON_EXECUTABLE}" "-c" "import PySide2"
        RESULT_VARIABLE pySideImportResult 
    )
    if (pySideImportResult EQUAL 0)
        set(pySideImportResult "PySide2")
        set(pySideUIC pyside2-uic python2-pyside2-uic pyside2-uic-2.7)
    else()
        set(pySideImportResult 0)
    endif()
else()
    set(pySideImportResult "PySide")
    set(pySideUIC pyside-uic python2-pyside-uic pyside-uic-2.7)
endif()

find_program(PYSIDEUICBINARY NAMES ${pySideUIC} HINTS ${PYSIDE_BIN_DIR})

if (pySideImportResult)
    if (EXISTS ${PYSIDEUICBINARY})
        message(STATUS "Found ${pySideImportResult}: with ${PYTHON_EXECUTABLE}, will use ${PYSIDEUICBINARY} for pyside-uic binary")
        set(PYSIDE_AVAILABLE True)
        if (pySideImportResult STREQUAL "PySide2")
            message(STATUS "Building against PySide2 is currently experimental.  "
                           "See https://bugreports.qt.io/browse/PYSIDE-357 if "
                           "'No module named Compiler' errors are encountered"
            )
        endif()
    else()
        message(STATUS "Found ${pySideImportResult} but NOT pyside-uic binary")
        set(PYSIDE_AVAILABLE False)
    endif()
else()
    if (PYSIDE_USE_PYSIDE2)
        message(STATUS "Did not find PySide2 with ${PYTHON_EXECUTABLE}")
    else()
        message(STATUS "Did not find PySide with ${PYTHON_EXECUTABLE}")
    endif()
    set(PYSIDE_AVAILABLE False)
endif()

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(PySide
    REQUIRED_VARS
        PYSIDE_AVAILABLE
        PYSIDEUICBINARY
)
