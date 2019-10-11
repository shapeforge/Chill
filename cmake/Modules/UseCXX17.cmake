cmake_minimum_required(VERSION 3.0.0)

# TODO: Set C Compiler Flags

include(CheckCXXCompilerFlag)

if(WIN32)
    set(COMPILER_SUPPORTS_CXX17 MSVC AND NOT MSVC_VERSION VERSION_LESS 1800)
else(WIN32)
    check_cxx_compiler_flag("-std=c++17" COMPILER_SUPPORTS_CXX17)
endif(WIN32)

if(COMPILER_SUPPORTS_CXX17)
  message(STATUS "Setting C++17 compiler flags (${CMAKE_CXX_COMPILER})")
  
  if(WIN32)
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std=c++17")
  else(WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
  endif(WIN32)

  if(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    if(WIN32)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /stdlib=libstdc++")
    else(WIN32)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libstdc++")
    endif(WIN32)
  endif()

else(COMPILER_SUPPORTS_CXX17)
  message(WARNING "The compiler ${CMAKE_CXX_COMPILER} has no C++17 support. Please use a different C++ compiler.")
endif(COMPILER_SUPPORTS_CXX17)

