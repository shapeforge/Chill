CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

ADD_SUBDIRECTORY(lua)
ADD_SUBDIRECTORY(luabind-deboostified)

#####################
# imgui integration #
#####################
SET(IMGUI_SOURCES
	imgui/imgui.cpp
  	imgui/imgui_draw.cpp
  	imgui/imgui_widgets.cpp
)

SET(IMGUI_HEADERS
	imgui/imgui.h
	imgui/imgui_internal.h
)

ADD_LIBRARY(imgui STATIC ${IMGUI_SOURCES} ${IMGUI_HEADERS})
SET_PROPERTY(TARGET imgui APPEND PROPERTY
   INTERFACE_INCLUDE_DIRECTORIES
 			${CMAKE_CURRENT_SOURCE_DIR}/imgui)

SET_TARGET_PROPERTIES(imgui PROPERTIES DEBUG_POSTFIX "-d")


###############################
# tinyfiledialogs integration #
###############################
SET(TINYFD_SOURCES
  	tinyfiledialogs/tinyfiledialogs.c
)

SET(TINYFD_HEADERS
  	tinyfiledialogs/tinyfiledialogs.h
)

ADD_LIBRARY(tinyfiledialogs STATIC ${TINYFD_SOURCES} ${TINYFD_HEADERS})
SET_PROPERTY(TARGET tinyfiledialogs APPEND PROPERTY
   INTERFACE_INCLUDE_DIRECTORIES
 			${CMAKE_CURRENT_SOURCE_DIR}/tinyfiledialogs)

SET_TARGET_PROPERTIES(tinyfiledialogs PROPERTIES DEBUG_POSTFIX "-d")
