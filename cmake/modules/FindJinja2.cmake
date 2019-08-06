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
# Jinja2 is a python library, ensure that it is available for use with our
# specified version of Python.
#
if (NOT PYTHON_EXECUTABLE)
    return()
endif()

execute_process(
    COMMAND 
        "${PYTHON_EXECUTABLE}" "-c" "import jinja2"
    RESULT_VARIABLE
        jinja2ImportResult 
)
if (jinja2ImportResult EQUAL 0)
    message(STATUS "Found Jinja2")
    set(JINJA2_FOUND True)
endif()


