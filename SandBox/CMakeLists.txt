cmake_minimum_required (VERSION 3.24)

project(SandBox)

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS 
	"Source/*.h" 
	"Source/*.cpp" 
)

add_executable(SandBox WIN32 ${SRC_FILES})

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SRC_FILES} )

EnableMultiProcessorCompilation(SandBox)
set_property(TARGET SandBox PROPERTY CXX_STANDARD 20)
set_property(TARGET SandBox PROPERTY VS_DEBUGGER_WORKING_DIRECTORY  "${CMAKE_SOURCE_DIR}/SandBox")


# Build folder
set_target_properties(SandBox
	PROPERTIES
	ARCHIVE_OUTPUT_DIRECTORY "${BUILD_FOLDER}/SandBox"
	LIBRARY_OUTPUT_DIRECTORY "${BUILD_FOLDER}/SandBox"
	RUNTIME_OUTPUT_DIRECTORY "${BUILD_FOLDER}/SandBox"
)

target_include_directories(SandBox PUBLIC 
	"Source" 
	"${CMAKE_SOURCE_DIR}/Athena/Source" 
	${THIRD_PARTY_DIR}
	${ENTT_INCLUDE_DIR}
	${IMGUI_INCLUDE_DIR}
	${IMGUIZMO_INCLUDE_DIR}
	${OPTICK_INCLUDE_DIR}
	${SPDLOG_INCLUDE_DIR}
)

target_link_libraries(SandBox Athena)
