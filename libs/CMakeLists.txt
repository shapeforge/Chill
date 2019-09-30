CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

ADD_SUBDIRECTORY(LibSL)
ADD_SUBDIRECTORY(lua)
ADD_SUBDIRECTORY(luabind-deboostified)

ADD_LIBRARY( tinyfiledialogs STATIC
  tinyfiledialogs/tinyfiledialogs.c
  tinyfiledialogs/tinyfiledialogs.h
)
