cmake_minimum_required(VERSION 3.0.0)

add_subdirectory(lua)
add_subdirectory(luabind-deboostified)

#####################
# imgui integration #
#####################
set(IMGUI_SOURCES
	imgui/imgui.cpp
  	imgui/imgui_draw.cpp
  	imgui/imgui_widgets.cpp
)

set(IMGUI_HEADERS
	imgui/imgui.h
	imgui/imgui_internal.h
)

add_library(imgui STATIC ${IMGUI_SOURCES} ${IMGUI_HEADERS})
set_property(TARGET imgui APPEND PROPERTY
   INTERFACE_INCLUDE_DIRECTORIES
 			${CMAKE_CURRENT_SOURCE_DIR}/imgui)

set_target_properties(imgui PROPERTIES DEBUG_POSTFIX "-d")


###############################
# tinyfiledialogs integration #
###############################
set(TINYFD_SOURCES
  	tinyfiledialogs/tinyfiledialogs.c
)

set(TINYFD_HEADERS
  	tinyfiledialogs/tinyfiledialogs.h
)

add_library(tinyfiledialogs STATIC ${TINYFD_SOURCES} ${TINYFD_HEADERS})
set_property(TARGET tinyfiledialogs APPEND PROPERTY
   INTERFACE_INCLUDE_DIRECTORIES
 			${CMAKE_CURRENT_SOURCE_DIR}/tinyfiledialogs)

set_target_properties(tinyfiledialogs PROPERTIES DEBUG_POSTFIX "-d")
