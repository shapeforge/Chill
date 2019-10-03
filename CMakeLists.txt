cmake_minimum_required(VERSION 3.0.0)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules/")

project(CHILL)

###################
# dev path macro #
###################
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/sourcePath.in"
  "${CMAKE_CURRENT_SOURCE_DIR}/chill/ChillEngine/SourcePath.h"
 )

if(WIN32)
  # only on Windows boxes (must appear at beginning of CMakeLists.txt file)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LARGEADDRESSAWARE")
  add_definitions(-DWIN32)
else(WIN32)
  # only on Linux boxes (must appear at beginning of CMakeLists.txt file)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--copy-dt-needed-entries")
endif(WIN32)

####################
# SDL2 integration #
####################
if(WIN32)
  set(SDL2_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/SDL2/")
endif(WIN32)

find_package(SDL2 REQUIRED)

if(SDL2_FOUND)
  include_directories(${SDL2_INCLUDE_DIRS})
  message(STATUS "Found SDL2 at " ${SDL2_DIR})
endif (SDL2_FOUND)

add_subdirectory(libs)
add_subdirectory(chill)
#ADD_SUBDIRECTORY(tests)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")