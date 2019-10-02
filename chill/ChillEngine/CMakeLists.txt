cmake_minimum_required(VERSION 3.0.0)

project(ChillEngine)

include(UseCXX17)

add_library(ChillEngine STATIC
	Processor.h
	ProcessingGraph.h
	LuaProcessor.h
	
	ProcessingGraph.cpp
	Processor.cpp
	GroupProcessors.cpp
	LuaProcessor.cpp

	Style.h
	UI.h
	UI.cpp
	NodeEditor.h
	NodeEditor.cpp

	VisualComment.h
	VisualComment.cpp

	IOTypes.h
	IOs.h
	IOs.cpp
  
	GraphSaver.h
	GraphSaver.cpp
  
	FileDialog.h
	FileDialog.cpp

	SourcePath.h
	Input.h
  )

target_link_libraries(ChillEngine
	${SDL2_LIBRARIES}
	imgui
	tinyfiledialogs
    lua
	luabind
)

if(UNIX)
  target_link_libraries(ChillEngine
    stdc++fs
  )
endif(UNIX)

###################
# dev path macro #
###################
set_property(TARGET ChillEngine APPEND PROPERTY
   INTERFACE_INCLUDE_DIRECTORIES
 			${CMAKE_CURRENT_SOURCE_DIR}
)

set_target_properties(ChillEngine PROPERTIES DEBUG_POSTFIX "-d")
set_target_properties(ChillEngine PROPERTIES ENABLE_EXPORTS ON)
