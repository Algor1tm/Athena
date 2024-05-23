cmake_minimum_required (VERSION 3.24)

project(Athena-Editor)

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS 
	"Source/*.h" 
	"Source/*.cpp" 
)

add_executable(Athena-Editor WIN32 ${SRC_FILES})

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SRC_FILES} )

EnableMultiProcessorCompilation(Athena-Editor)
set_property(TARGET Athena-Editor PROPERTY CXX_STANDARD 20)
set_property(TARGET Athena-Editor PROPERTY VS_DEBUGGER_WORKING_DIRECTORY  "${CMAKE_SOURCE_DIR}/Athena-Editor")
set_property(TARGET Athena-Editor PROPERTY FOLDER Tools)


# Build folder
set_target_properties(Athena-Editor
	PROPERTIES
	ARCHIVE_OUTPUT_DIRECTORY "${BUILD_FOLDER}/Athena-Editor"
	LIBRARY_OUTPUT_DIRECTORY "${BUILD_FOLDER}/Athena-Editor"
	RUNTIME_OUTPUT_DIRECTORY "${BUILD_FOLDER}/Athena-Editor"
)

target_include_directories(Athena-Editor PRIVATE 
	"Source" 
	"${CMAKE_SOURCE_DIR}/Athena/Source" 
	${THIRD_PARTY_DIR}
	${ENTT_INCLUDE_DIR}
	${IMGUI_INCLUDE_DIR}
	${IMGUIZMO_INCLUDE_DIR}
	${OPTICK_INCLUDE_DIR}
	${SPDLOG_INCLUDE_DIR}
	${MSDF_ATLAS_GEN_INCLUDE_DIR}
	${MSDF_GEN_INCLUDE_DIR}
)

target_link_libraries(Athena-Editor PRIVATE Athena)
