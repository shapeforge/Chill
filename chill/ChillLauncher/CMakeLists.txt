cmake_minimum_required(VERSION 3.0.0)

project(ChillLauncher)

include(UseCXX17)

add_executable(ChillLauncher
  launcher.cpp
  ../../ressources/main.rc
)

target_link_libraries(ChillLauncher
  ChillEngine
)

set_property(TARGET ChillLauncher APPEND PROPERTY
   INTERFACE_INCLUDE_DIRECTORIES
 			${CMAKE_CURRENT_SOURCE_DIR}
)
set_target_properties(ChillLauncher PROPERTIES DEBUG_POSTFIX "-d")
