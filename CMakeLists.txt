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
project(redfst)
find_package(OpenMP)
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
option(REDFST_STATIC "Static library" OFF)
add_subdirectory(src)
add_subdirectory(examples)