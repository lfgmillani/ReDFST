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

include_directories(${CMAKE_SOURCE_DIR}/src/libredfst)
link_directories(${CMAKE_BINARY_DIR}/src/libredfst)
add_executable(redfst_example example.c)
if(REDFST_STATIC)
  add_dependencies(redfst_example libredfst_static)
  target_link_libraries(redfst_example cpufreq)
else(REDFST_STATIC)
  add_dependencies(redfst_example libredfst_dynamic)
endif(REDFST_STATIC)
target_link_libraries(redfst_example redfst)

