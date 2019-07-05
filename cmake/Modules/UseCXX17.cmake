CMake_Minimum_Required(VERSION 2.8.0)

# TODO: Set C Compiler Flags

Message(STATUS "Setting C++17 compiler flags")

Include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++17" COMPILER_SUPPORTS_CXX17)

If(COMPILER_SUPPORTS_CXX17)
	Set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
        Set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -lc++fs -lc++abi")
Else()
	If(NOT MSVC OR MSVC_VERSION VERSION_LESS 1800)
		Message(WARNING "The compiler ${CMAKE_CXX_COMPILER} has no C++17 support. Please use a different C++ compiler.")
	Endif()
Endif()
