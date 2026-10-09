cmake_minimum_required (VERSION 3.24)

######################################
##### MAIN PROJECT
######################################

project(SandBox)

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS 
	"Source/*.h" 
	"Source/*.cpp" 
)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SRC_FILES} )

add_executable(SandBox WIN32 ${SRC_FILES})

EnableMPCompilation(SandBox)
set_target_properties(SandBox
	PROPERTIES
	CXX_STANDARD 20
	VS_DEBUGGER_WORKING_DIRECTORY  "${CMAKE_SOURCE_DIR}/SandBox"
)

set_target_properties(SandBox
	PROPERTIES
	ARCHIVE_OUTPUT_DIRECTORY "${BUILD_FOLDER}/SandBox"
	LIBRARY_OUTPUT_DIRECTORY "${BUILD_FOLDER}/SandBox"
	RUNTIME_OUTPUT_DIRECTORY "${BUILD_FOLDER}/SandBox"
)

target_include_directories(SandBox PUBLIC 
	"Source" 
	"${CMAKE_SOURCE_DIR}/Athena/Source" 
)

######################################
##### DEPENDENCIES
######################################

IncludeDependenciesDirs(SandBox)
target_link_libraries(SandBox Athena)
