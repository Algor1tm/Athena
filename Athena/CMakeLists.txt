cmake_minimum_required (VERSION 3.16)

function(AddLibrary TARGET_NAME)
    add_subdirectory("${THIRD_PARTY_DIR}/${TARGET_NAME}")
    EnableMultiProcessorCompilation(${TARGET_NAME})
    set_property(TARGET ${TARGET_NAME} PROPERTY FOLDER Dependencies)

    set_target_properties(${TARGET_NAME}
	PROPERTIES
	ARCHIVE_OUTPUT_DIRECTORY "${BUILD_FOLDER}/ThirdParty/${TARGET_NAME}"
	LIBRARY_OUTPUT_DIRECTORY "${BUILD_FOLDER}/ThirdParty/${TARGET_NAME}"
	RUNTIME_OUTPUT_DIRECTORY "${BUILD_FOLDER}/ThirdParty/${TARGET_NAME}")

    target_link_libraries(Athena PRIVATE ${TARGET_NAME})
endfunction()


project(Athena)

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS 
	Athena.def
	"Source/*.h" 
	"Source/*.cpp" 
	"ThirdParty/pybind11/include/*.h"
	"ThirdParty/pybind11/include/*.cpp"
	"ThirdParty/stb_image/stb_image.h"
	"ThirdParty/stb_image/stb_image.cpp"
	"ThirdParty/entt/entt.h"
	"ThirdParty/ImGuizmo/ImGuizmo.h"
	"ThirdParty/ImGuizmo/ImGuizmo.cpp"
)

add_library(Athena SHARED ${SRC_FILES})

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SRC_FILES} )

EnableMultiProcessorCompilation(Athena)
set_property(TARGET Athena PROPERTY CXX_STANDARD 17)
set_property(TARGET Athena PROPERTY FOLDER Core)
set_property(TARGET Athena PROPERTY DEBUG_POSTFIX "")

target_compile_definitions(Athena PRIVATE
	$<$<CONFIG:Debug>:ATN_DEBUG>
	$<$<CONFIG:Release>:ATN_RELEASE>
	ATN_SIMD
	ATN_BUILD_DLL
)

if(ATN_PY_DEBUG)
	target_compile_definitions(Athena PRIVATE
		$<$<CONFIG:Debug>:Py_DEBUG>
	)
endif(ATN_PY_DEBUG)

# Build folder
set_target_properties(Athena
	PROPERTIES
	ARCHIVE_OUTPUT_DIRECTORY "${BUILD_FOLDER}/Athena"
	LIBRARY_OUTPUT_DIRECTORY "${BUILD_FOLDER}/Athena"
	RUNTIME_OUTPUT_DIRECTORY "${BUILD_FOLDER}/Athena"
)

add_custom_command(TARGET Athena POST_BUILD 
  COMMAND "${CMAKE_COMMAND}" -E copy 
     "$<TARGET_FILE:Athena>"
     "${BUILD_FOLDER}/Athena-Editor/$<CONFIGURATION>/$<TARGET_FILE_NAME:Athena>" )

add_custom_command(TARGET Athena POST_BUILD 
  COMMAND "${CMAKE_COMMAND}" -E copy 
     "$<TARGET_FILE:Athena>"
     "${BUILD_FOLDER}/SandBox/$<CONFIGURATION>/$<TARGET_FILE_NAME:Athena>" )

# Precompile Header
target_precompile_headers(Athena PRIVATE
	<memory> <algorithm> <functional> <utility> <chrono>
	<iomanip> <thread> <filesystem> <limits> <random>
	<numeric> <math.h> <cmath> <iostream> <sstream>
	<fstream> <array> <vector> <string> <unordered_map> <unordered_set>

	Source/Athena/Core/Core.h
	Source/Athena/Core/Log.h
)

target_include_directories(Athena PRIVATE 
	"Source" 
	${ASSIMP_INCLUDE_DIR}
	${THIRD_PARTY_DIR}
	${BOX2D_INCLUDE_DIR}
	${GLAD_INCLUDE_DIR}
	${GLFW_INCLUDE_DIR}
	${IMGUI_INCLUDE_DIR}
	${PYBIND11_INCLUDE_DIR}
	${SPDLOG_INCLUDE_DIR}
	${YAML_CPP_INCLUDE_DIR})


AddLibrary("Box2D")
AddLibrary(glad)
AddLibrary(GLFW)
AddLibrary(ImGui)
AddLibrary(spdlog)

AddLibrary(yaml-cpp)
target_compile_definitions(yaml-cpp PUBLIC YAML_CPP_STATIC_DEFINE) # Static compile

add_subdirectory("${THIRD_PARTY_DIR}/pybind11")
target_link_libraries(Athena PRIVATE pybind11::embed)


# Windows specific code
if(WIN32)
	target_link_libraries(Athena PRIVATE "${CMAKE_SOURCE_DIR}/Athena/ThirdParty/assimp/libs/Windows/assimp-vc143-mt.lib")

	target_link_libraries(Athena PRIVATE opengl32.lib)
	target_compile_definitions(Athena PUBLIC ATN_WINDOWS)
endif(WIN32)

# Linux specific code
if(UNIX AND NOT APPLE)
	set_property(TARGET Athena PROPERTY POSITION_INDEPENDENT_CODE ON)
	target_link_libraries(Athena PRIVATE opengl32.lib)
    target_compile_definitions(Athena PUBLIC ATN_LINUX)
endif(UNIX AND NOT APPLE)
