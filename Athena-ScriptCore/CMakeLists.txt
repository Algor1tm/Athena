cmake_minimum_required (VERSION 3.16)

project(AthenaScriptCore)

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS 
	"Source/*.h" 
	"Source/*.cpp"
)

file(GLOB_RECURSE PYTHON_FILES CONFIGURE_DEPENDS 
	"${SCRIPTS_FOLDER}/Athena.py"
	"${SCRIPTS_FOLDER}/Internal/*.py")

pybind11_add_module(AthenaScriptCore ${SRC_FILES} ${PYTHON_FILES})

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SRC_FILES} )
source_group(TREE "${SCRIPTS_FOLDER}/../" FILES ${PYTHON_FILES} )

EnableMultiProcessorCompilation(AthenaScriptCore)
set_property(TARGET AthenaScriptCore PROPERTY CXX_STANDARD 17)
set_property(TARGET AthenaScriptCore PROPERTY FOLDER Core)

target_compile_definitions(AthenaScriptCore PUBLIC
	$<$<CONFIG:Debug>:ATN_DEBUG>
	$<$<CONFIG:Release>:ATN_RELEASE>
)

# Build folder
set_target_properties(AthenaScriptCore
	PROPERTIES
	ARCHIVE_OUTPUT_DIRECTORY "${BUILD_FOLDER}/AthenaScriptCore"
	LIBRARY_OUTPUT_DIRECTORY "${BUILD_FOLDER}/AthenaScriptCore"
	RUNTIME_OUTPUT_DIRECTORY "${BUILD_FOLDER}/AthenaScriptCore"
)

target_include_directories(AthenaScriptCore PUBLIC 
${PYBIND11_INCLUDE_DIR}
"${CMAKE_SOURCE_DIR}/Athena/Source"
${SPDLOG_INCLUDE_DIR})

target_link_libraries(AthenaScriptCore PRIVATE Athena)

add_custom_command(TARGET AthenaScriptCore POST_BUILD 
  COMMAND "${CMAKE_COMMAND}" -E copy 
     "$<TARGET_FILE:AthenaScriptCore>"
     "${SCRIPTS_FOLDER}/Internal" )

add_custom_command(TARGET AthenaScriptCore POST_BUILD 
  COMMAND "${CMAKE_COMMAND}" -E copy 
     "$<TARGET_FILE:Athena>"
     "${SCRIPTS_FOLDER}/Internal" )
