# Copyright Codeplay Software Ltd.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use these files except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

#########################
#  FindDPCPP.cmake
#########################

include_guard()

include(FindPackageHandleStandardArgs)

get_filename_component(DPCPP_BIN_DIR ${CMAKE_CXX_COMPILER} DIRECTORY)
find_library(DPCPP_LIB NAMES sycl PATHS "${DPCPP_BIN_DIR}/../lib")

find_package_handle_standard_args(DPCPP
  FOUND_VAR     DPCPP_FOUND
  REQUIRED_VARS DPCPP_LIB
)

if(NOT DPCPP_FOUND)
  return()
endif()

set(DPCPP_SYCL_TARGET "spir64" CACHE STRING "SYCL TARGET")

mark_as_advanced(DPCPP_FOUND
                 DPCPP_LIB
)

if(DPCPP_FOUND AND NOT TARGET DPCPP::DPCPP)
  set(CMAKE_CXX_STANDARD 17)
  add_library(DPCPP::DPCPP INTERFACE IMPORTED)
  set_target_properties(DPCPP::DPCPP PROPERTIES
    INTERFACE_COMPILE_OPTIONS "-fsycl;-fsycl-targets=${DPCPP_SYCL_TARGET};-Xclang;-cl-mad-enable;-fsycl-unnamed-lambda"
    INTERFACE_LINK_OPTIONS "-fsycl;-fsycl-targets=${DPCPP_SYCL_TARGET}"
    INTERFACE_LINK_LIBRARIES ${DPCPP_LIB}
    INTERFACE_INCLUDE_DIRECTORIES "${DPCPP_BIN_DIR}/../include/sycl;${DPCPP_BIN_DIR}/../include")
endif()

function(add_sycl_to_target)
  set(options)
  set(one_value_args TARGET)
  cmake_parse_arguments(SNN_ADD_SYCL
    "${options}"
    "${one_value_args}"
    "${multi_value_args}"
    ${ARGN}
  )
  target_compile_options(${SNN_ADD_SYCL_TARGET} PRIVATE "$<$<CONFIG:Release>:-O3>")
  target_include_directories(${SNN_ADD_SYCL_TARGET} PUBLIC ${DPCPP_INCLUDE_DIRS})
  target_link_libraries(${SNN_ADD_SYCL_TARGET} PUBLIC DPCPP::DPCPP)                        
endfunction()
