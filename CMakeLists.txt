CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

#SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules/")

PROJECT(CHILL)

CONFIGURE_FILE(
  "${CMAKE_CURRENT_SOURCE_DIR}/sourcePath.in"
  "${CMAKE_CURRENT_SOURCE_DIR}/chill/ChillEngine/SourcePath.h"
 )

IF (WIN32)
  # only on Windows boxes (must appear at beginning of CMakeLists.txt file)
  SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LARGEADDRESSAWARE")
  ADD_DEFINITIONS(-DWIN32)
ELSE (WIN32)
  # only on Linux boxes (must appear at beginning of CMakeLists.txt file)
  SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--copy-dt-needed-entries")
ENDIF()

###############################
# SDL2 integration #
###############################
if (WIN32)
  SET(SDL2_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/SDL2/")
endif (WIN32)
FIND_PACKAGE(SDL2 REQUIRED)

if (SDL2_FOUND)
  INCLUDE_DIRECTORIES(${SDL2_INCLUDE_DIRS})
  MESSAGE(STATUS "Found SDL2 at " ${SDL2_DIR})
else(SDL2_FOUND)
  MESSAGE("SDL2_LIBRARIES not found !")
endif (SDL2_FOUND)


ADD_SUBDIRECTORY(libs)
ADD_SUBDIRECTORY(chill)
#ADD_SUBDIRECTORY(tests)

#IF(WIN32)
#  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")
#  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /stdlib=libc++")
#ELSE()
#  Set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
#  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
#ENDIF()
