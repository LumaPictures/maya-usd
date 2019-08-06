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
    message(FATAL_ERROR "Unable to find Python executable - PyOpenGL not present")
    return()
endif()

execute_process(
    COMMAND 
        "${PYTHON_EXECUTABLE}" "-c" "from OpenGL import *"
    RESULT_VARIABLE
        pyopenglImportResult 
)

if (pyopenglImportResult EQUAL 0)
    message(STATUS "Found PyOpenGL")
    set(PYOPENGL_AVAILABLE True)
endif()

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(PyOpenGL
    REQUIRED_VARS
        PYOPENGL_AVAILABLE
)
