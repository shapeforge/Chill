CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

if (EMSCRIPTEN)

SET(CMAKE_EXE_LINKER_FLAGS "-s TOTAL_MEMORY=128000000")

else (EMSCRIPTEN)

# only on Linux boxes (must appear at beginning of CMakeLists.txt file)
SET(CMAKE_EXE_LINKER_FLAGS "-Wl,--copy-dt-needed-entries")
# only on Windows boxes (must appear at beginning of CMakeLists.txt file)
SET(CMAKE_EXE_LINKER_FLAGS "/LARGEADDRESSAWARE")

endif (EMSCRIPTEN)

if (WIN32)
ADD_DEFINITIONS(-DWIN32)
endif (WIN32)

ADD_SUBDIRECTORY(libs)
ADD_SUBDIRECTORY(chill)
ADD_SUBDIRECTORY(tests)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")