CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(ChillLauncher)

include(UseCXX17)

ADD_EXECUTABLE( ChillLauncher
  launcher.cpp
  ../../ressources/main.rc
)

TARGET_LINK_LIBRARIES( ChillLauncher
  ChillEngine
)

IF(UNIX)
  TARGET_LINK_LIBRARIES( ChillLauncher
    stdc++fs
#    c++fs c++abi
  )
ENDIF(UNIX)

SET_PROPERTY(TARGET ChillLauncher APPEND PROPERTY
   INTERFACE_INCLUDE_DIRECTORIES
 			${CMAKE_CURRENT_SOURCE_DIR}
)
SET_TARGET_PROPERTIES(ChillLauncher PROPERTIES DEBUG_POSTFIX "-d")
