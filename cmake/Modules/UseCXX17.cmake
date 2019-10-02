cmake_minimum_required(VERSION 3.0.0)


# TODO: Set C Compiler Flags

message(STATUS "Setting C++17 compiler flags")

include(CheckCXXCompilerFlag)
if(WIN32)
	#check_cxx_compiler_flag("/std=c++17" COMPILER_SUPPORTS_CXX17)
else(WIN32)
	check_cxx_compiler_flag("-std=c++17" COMPILER_SUPPORTS_CXX17)
endif(WIN32)

if(COMPILER_SUPPORTS_CXX17)
	if(WIN32)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std=c++17")
  		if(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /stdlib=libstdc++")
			#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
  		endif()
  	else(WIN32)
  		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
  		if(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libstdc++")
			#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
  		endif()
	endif(WIN32)
else()
	if(NOT MSVC OR MSVC_VERSION VERSION_LESS 1800)
		message(WARNING "The compiler ${CMAKE_CXX_COMPILER} has no C++17 support. Please use a different C++ compiler.")
	endif()
endif()
