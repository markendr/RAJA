###############################################################################
# Copyright (c) 2016-21, Lawrence Livermore National Security, LLC
# and RAJA project contributors. See the RAJA/LICENSE file for details.
#
# SPDX-License-Identifier: (BSD-3-Clause)
###############################################################################

set(RAJA_COMPILER "RAJA_COMPILER_ICC" CACHE STRING "")

option(RAJA_USE_PAPI "Enable PAPI performance monitoring" OFF)

if(RAJA_USE_PAPI)
  message("PAPI enabled")
  set(COMMON_FLAGS "${COMMON_FLAGS} -DRAJA_USE_PAPI -I$ENV{HOME}/papi/include")
  set(CMAKE_EXE_LINKER_FLAGS "-lpapi -L$ENV{HOME}/papi/lib" CACHE STRING "")
endif()

set(CMAKE_CXX_FLAGS_RELEASE "${COMMON_FLAGS} -O3 -march=native -ansi-alias -diag-disable cpu-dispatch" CACHE STRING "")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${COMMON_FLAGS} -O3 -g -march=native -ansi-alias -diag-disable cpu-dispatch" CACHE STRING "")
set(CMAKE_CXX_FLAGS_DEBUG "${COMMON_FLAGS} -O0 -g" CACHE STRING "")

set(RAJA_DATA_ALIGN 64 CACHE STRING "")

set(RAJA_HOST_CONFIG_LOADED On CACHE BOOL "")
