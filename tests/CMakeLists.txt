CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(ChillTest)

ENABLE_TESTING() # once

add_library( ChillTest SHARED
  stdafx.cpp
  stdafx.h
  targetver.h
  graphTest.cpp
)

TARGET_LINK_LIBRARIES( ChillTest
  ChillEngine
  LibSL
)

set_target_properties(ChillTest PROPERTIES LINK_FLAGS "/LIBPATH:$(VCInstallDir)UnitTest/lib")
set_target_properties(ChillTest PROPERTIES COMPILE_FLAGS "/I$(VCInstallDir)UnitTest/include")

ADD_TEST( Test
  ChillTest
)


