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
  global.c
  init.c
  monitor.c
  perf.c
  profile.c
  redfst_omp.c
  region.c
  show.c
  util.c)

set(LIBREDFST_HEADERS
  redfst.h)

#maximum number of code regions; as 64bit integers are used with this, it cannot be higher than 64.
set(REDFSTLIB_MAX_REGIONS 64)
#maximum number of threads supported
set(REDFSTLIB_MAX_THREADS 32)
#polling frequency for the monitor (in microseconds)
set(REDFSTLIB_MONITOR_FREQUENCY 100)
#if defined, the library will target OpenMP. at the moment that's the only option.
set(REDFSTLIB_OMP ON)
#if defined, frequency is decided on a per-core basis. otherwise, its per-cpu.
set(REDFSTLIB_FREQ_PER_CORE ON)
#if REDFSTLIB_FREQ_PER_CORE was not defined, these variables define
#which cores will have their frequencies changed. all other cores will
#be kept at the low frequency.
set(REDFSTLIB_CPU0 0)
set(REDFSTLIB_CPU1 1)

include_directories(${CMAKE_BINARY_DIR})
add_library(libredfst_static STATIC ${LIBREDFST_SOURCES})
set_target_properties(libredfst_static PROPERTIES OUTPUT_NAME redfst)

add_library(libredfst_dynamic SHARED ${LIBREDFST_SOURCES})
set_target_properties(libredfst_dynamic PROPERTIES OUTPUT_NAME redfst)
target_link_libraries(libredfst_dynamic cpufreq)

install(TARGETS libredfst_static DESTINATION lib)
install(TARGETS libredfst_dynamic DESTINATION lib)
install(FILES ${LIBREDFST_HEADERS} DESTINATION include)

configure_file("${CMAKE_HOME_DIRECTORY}/src/libredfst/libredfst_config.h.in" "${CMAKE_BINARY_DIR}/libredfst_config.h" @ONLY IMMEDIATE)
