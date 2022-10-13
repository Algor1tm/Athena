cmake_minimum_required (VERSION 3.16)

project(Athena)

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS 
	Athena.def
	"Source/*.h" 
	"Source/*.cpp" 
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

target_compile_definitions(Athena PRIVATE
	$<$<CONFIG:Debug>:ATN_DEBUG>
	$<$<CONFIG:Release>:ATN_RELEASE>
	ATN_SIMD
	ATN_BUILD_DLL
)

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
	<numeric> <math.h> <cmath> <xmmintrin.h> <iostream> <sstream>
	<fstream> <array> <vector> <string> <unordered_map> <unordered_set>

	Source/Athena/Core/Core.h
	Source/Athena/Core/Log.h
)

if(WIN32)
	target_precompile_headers(Athena PRIVATE <Windows.h> <d3d11.h>)
endif(WIN32)


target_include_directories(Athena PRIVATE 
	"Source" 
	${THIRD_PARTY_DIR}
	${BOX2D_INCLUDE_DIR}
	${GLAD_INCLUDE_DIR}
	${GLFW_INCLUDE_DIR}
	${IMGUI_INCLUDE_DIR}
	${SPDLOG_INCLUDE_DIR}
	${YAML_CPP_INCLUDE_DIR})

target_link_libraries(Athena PRIVATE
	Box2D
	glad
	GLFW
	ImGui
	spdlog
	yaml-cpp
)

set_property(TARGET Box2D PROPERTY FOLDER Dependencies)
set_property(TARGET glad PROPERTY FOLDER Dependencies)
set_property(TARGET GLFW PROPERTY FOLDER Dependencies)
set_property(TARGET ImGui PROPERTY FOLDER Dependencies)
set_property(TARGET spdlog PROPERTY FOLDER Dependencies)
set_property(TARGET yaml-cpp PROPERTY FOLDER Dependencies)

EnableMultiProcessorCompilation(Box2D)
EnableMultiProcessorCompilation(glad)
EnableMultiProcessorCompilation(GLFW)
EnableMultiProcessorCompilation(ImGui)
EnableMultiProcessorCompilation(spdlog)
EnableMultiProcessorCompilation(yaml-cpp)

target_compile_definitions(yaml-cpp PUBLIC YAML_CPP_STATIC_DEFINE) # Static compile

# Windows specific code
if(WIN32)
	target_link_libraries(Athena PRIVATE d3d11.lib)
	target_link_libraries(Athena PRIVATE DXGI.lib)
	target_compile_definitions(Athena PUBLIC ATN_WINDOWS)
endif(WIN32)

# Linux specific code
if(UNIX AND NOT APPLE)
	set_property(TARGET Athena PROPERTY POSITION_INDEPENDENT_CODE ON)
	target_link_libraries(Athena PRIVATE opengl32.lib)
    target_compile_definitions(Athena PUBLIC ATN_LINUX)
endif(UNIX AND NOT APPLE)
