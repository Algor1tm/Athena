cmake_minimum_required (VERSION 3.16)

project(Athena-Editor)

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS 
	"Source/*.h" 
	"Source/*.cpp" 
)

add_executable(Athena-Editor WIN32 ${SRC_FILES})

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SRC_FILES} )

EnableMultiProcessorCompilation(Athena-Editor)
set_property(TARGET Athena-Editor PROPERTY CXX_STANDARD 17)
set_property(TARGET Athena-Editor PROPERTY VS_DEBUGGER_WORKING_DIRECTORY  "${CMAKE_SOURCE_DIR}/Athena-Editor")

target_compile_definitions(Athena-Editor PUBLIC
  $<$<CONFIG:Debug>:ATN_DEBUG>
  $<$<CONFIG:Release>:ATN_RELEASE>
  ATN_SIMD
)

# Build folder
set_target_properties(Athena-Editor
	PROPERTIES
	ARCHIVE_OUTPUT_DIRECTORY "${BUILD_FOLDER}/Athena-Editor"
	LIBRARY_OUTPUT_DIRECTORY "${BUILD_FOLDER}/Athena-Editor"
	RUNTIME_OUTPUT_DIRECTORY "${BUILD_FOLDER}/Athena-Editor"
)

target_include_directories(Athena-Editor PUBLIC 
	"Source" 
	"${CMAKE_SOURCE_DIR}/Athena/Source" 
	${THIRD_PARTY_DIR}
	${ENTT_INCLUDE_DIR}
	${IMGUI_INCLUDE_DIR}
	${IMGUIZMO_INCLUDE_DIR}
	${SPDLOG_INCLUDE_DIR}
)

target_link_libraries(Athena-Editor Athena)
