# This file is part of ReDFST
#
# ReDFST is free software: you can redistribute it and/or modify it
# under the terms of the GNU Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# ReDFST is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Public License for more details.
#
# You should have received a copy of the GNU Public License
# along with ReDFST. If not, see <http://www.gnu.org/licenses/>.
cmake_minimum_required(VERSION 2.8)

set(LIBREDFST_SOURCES
  control.c
  energy.c
  global.c
  hw.c
  init.c
  likwid.c
  monitor.c
  msr.c
  perf.c
  powercap.c
  profile.c
  redfst_omp.c
  region-all.c
  region.c
  show.c
  trace.c
  util.c
)

#maximum number of code regions; as 64bit integers are used with this, it cannot be higher than 64.
set(REDFSTLIB_MAX_REGIONS 64)
#maximum number of threads supported
set(REDFSTLIB_MAX_THREADS 32)
#polling frequency for the monitor (in microseconds)
set(REDFSTLIB_MONITOR_FREQUENCY 100)
#if defined, the library will target OpenMP. at the moment that's the only option.
set(REDFSTLIB_OMP ON)
#if defined, small functions will be implemented in the header
set(REDFST_FUN_IN_H OFF)
#if defined, frequency is decided on a per-core basis. otherwise, its per-cpu.
set(REDFSTLIB_FREQ_PER_CORE ON)
#if REDFSTLIB_FREQ_PER_CORE was not defined, these variables define
#which cores will have their frequencies changed. all other cores will
#be kept at the low frequency.
set(REDFSTLIB_CPU0 0)
set(REDFSTLIB_CPU1 1)
#if defined, all calls to redfst_region(_all) are traced
set(REDFSTLIB_TRACE OFF)

include_directories(${CMAKE_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
if(REDFST_STATIC)
  add_library(libredfst_static STATIC ${LIBREDFST_SOURCES})
  set_target_properties(libredfst_static PROPERTIES OUTPUT_NAME redfst)
    target_link_libraries(libredfst_static cpupower)
  install(TARGETS libredfst_static DESTINATION lib)
else(REDFST_STATIC)
  add_library(libredfst_dynamic SHARED ${LIBREDFST_SOURCES})
  set_target_properties(libredfst_dynamic PROPERTIES OUTPUT_NAME redfst)
  target_link_libraries(libredfst_dynamic cpupower)
  install(TARGETS libredfst_dynamic DESTINATION lib)
  install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/redfst-default.h DESTINATION include RENAME redfst.h)
endif(REDFST_STATIC)

file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/redfst-default.h" DESTINATION "${CMAKE_BINARY_DIR}/src")
file(RENAME "${CMAKE_BINARY_DIR}/src/redfst-default.h" "${CMAKE_BINARY_DIR}/src/redfst.h")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/config.h.in" "${CMAKE_BINARY_DIR}/config.h" @ONLY IMMEDIATE)

