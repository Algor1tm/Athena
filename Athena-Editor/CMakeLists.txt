cmake_minimum_required (VERSION 3.24)

######################################
##### MAIN PROJECT
######################################

project(Athena-Editor)

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS 
	"Source/*.h" 
	"Source/*.cpp" 
)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SRC_FILES} )

add_executable(Athena-Editor WIN32 ${SRC_FILES})

EnableMPCompilation(Athena-Editor)
set_target_properties(Athena-Editor
	PROPERTIES
	CXX_STANDARD 20
	FOLDER Tools
	VS_DEBUGGER_WORKING_DIRECTORY  "${CMAKE_SOURCE_DIR}/Athena-Editor"
)

set_target_properties(Athena-Editor
	PROPERTIES
	ARCHIVE_OUTPUT_DIRECTORY "${BUILD_FOLDER}/Athena-Editor"
	LIBRARY_OUTPUT_DIRECTORY "${BUILD_FOLDER}/Athena-Editor"
	RUNTIME_OUTPUT_DIRECTORY "${BUILD_FOLDER}/Athena-Editor"
)

target_include_directories(Athena-Editor PRIVATE 
	"Source" 
	"${CMAKE_SOURCE_DIR}/Athena/Source" 
)

######################################
##### DEPENDENCIES
######################################

IncludeDependenciesDirs(Athena-Editor)
target_link_libraries(Athena-Editor PRIVATE Athena)
