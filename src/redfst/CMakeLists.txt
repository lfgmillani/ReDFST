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

set(REDFST_SOURCES
  main.c
  launch.c)

#maximum number of cores supported
set(REDFSTBIN_MAX_CORES 64)
# frequency at which the energy counters should be read.
# high values increase overhead.
# with lower values the program takes longer to react to events such as the
# target exiting or stdin commands.
set(REDFSTBIN_POLLING_FREQUENCY 100)

include_directories(${CMAKE_BINARY_DIR})
add_executable(redfst ${REDFST_SOURCES})
target_link_libraries(redfst m)
install(PROGRAMS ${CMAKE_BINARY_DIR}/src/redfst/redfst DESTINATION bin)

configure_file("${CMAKE_HOME_DIRECTORY}/src/redfst/redfst_config.h.in" "${CMAKE_BINARY_DIR}/redfst_config.h" @ONLY IMMEDIATE)

