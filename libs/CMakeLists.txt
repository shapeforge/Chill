cmake_minimum_required(VERSION 3.0.0)

##########
# openGL #
##########
set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)

########
# gl3w #
########
set(GL3W_SOURCES
  gl3w/GL/gl3w.c
)

set(GL3W_HEADERS
  gl3w/GL/gl3w.h
  gl3w/GL/glcorearb.h
)

add_library(gl3w STATIC ${GL3W_SOURCES} ${GL3W_HEADERS})
set_property(
  TARGET gl3w
  APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/gl3w
)
set_target_properties(gl3w PROPERTIES DEBUG_POSTFIX "-d")

target_link_libraries(gl3w
  ${OPENGL_LIBRARIES}
)

#####################
# imgui integration #
#####################
add_definitions(-DIMGUI_IMPL_OPENGL_LOADER_GL3W)

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
set_property(
  TARGET imgui
  APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/imgui
)

set_target_properties(imgui PROPERTIES DEBUG_POSTFIX "-d")


##################
# imgui bindings #
##################
set(IMGUI_BINDINGS_SOURCES
  imgui/examples/imgui_impl_opengl3.cpp
  imgui/examples/imgui_impl_sdl.cpp
)

set(IMGUI_BINDINGS_HEADERS
  imgui/examples/imgui_impl_opengl3.h
  imgui/examples/imgui_impl_sdl.h
)

add_library(imgui_bindings STATIC ${IMGUI_BINDINGS_SOURCES} ${IMGUI_BINDINGS_HEADERS})
set_property(
  TARGET imgui_bindings
  APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/imgui/examples
)

set_target_properties(imgui_bindings PROPERTIES DEBUG_POSTFIX "-d")

target_link_libraries(imgui_bindings
  imgui
  gl3w
  ${SDL2_LIBRARIES}
)

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
set_property(
  TARGET tinyfiledialogs
  APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/tinyfiledialogs
)

set_target_properties(tinyfiledialogs PROPERTIES DEBUG_POSTFIX "-d")

#################
# Lua libraries #
#################
add_subdirectory(lua)
add_subdirectory(luabind-deboostified)
