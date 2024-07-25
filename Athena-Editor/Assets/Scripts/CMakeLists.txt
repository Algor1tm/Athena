cmake_minimum_required (VERSION 3.24)

include(Config.cmake)

set(BUILD_FOLDER "${CMAKE_SOURCE_DIR}/Build/Binaries")
set(SHADERS_FOLDER "${ATHENA_SOURCE_DIR}/Athena/EngineResources/ShaderPack")

# Enable in cross-platform way multi-processor compilation
macro(EnableMultiProcessorCompilation TARGET_NAME)
    if(MSVC)
	    target_compile_options(${TARGET_NAME} PUBLIC /MP)
    endif(MSVC)
    #TODO: support other compilers
endmacro()

project (${PROJECT_NAME})

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)


file(GLOB_RECURSE SRC_SCRIPT_FILES CONFIGURE_DEPENDS 
	"Source/*.h" 
	"Source/*.cpp" 
)

add_library(${PROJECT_NAME} SHARED ${SRC_SCRIPT_FILES})

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SRC_SCRIPT_FILES} )
EnableMultiProcessorCompilation(${PROJECT_NAME})
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)

target_compile_definitions(${PROJECT_NAME} PUBLIC
	$<$<CONFIG:Debug>:ATN_DEBUG>
	$<$<CONFIG:Release>:ATN_RELEASE>
	ATN_SIMD
	STBIW_WINDOWS_UTF8
	_SILENCE_ALL_MS_EXT_DEPRECATION_WARNINGS
)

# Build folder (debug and release have the same dir)
set_target_properties(${PROJECT_NAME}
	PROPERTIES
	ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${BUILD_FOLDER}/${PROJECT_NAME}"
	LIBRARY_OUTPUT_DIRECTORY_DEBUG "${BUILD_FOLDER}/${PROJECT_NAME}"
	RUNTIME_OUTPUT_DIRECTORY_DEBUG "${BUILD_FOLDER}/${PROJECT_NAME}"
	ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${BUILD_FOLDER}/${PROJECT_NAME}"
	LIBRARY_OUTPUT_DIRECTORY_RELEASE "${BUILD_FOLDER}/${PROJECT_NAME}"
	RUNTIME_OUTPUT_DIRECTORY_RELEASE "${BUILD_FOLDER}/${PROJECT_NAME}"
)

set(THIRD_PARTY_DIR "${ATHENA_SOURCE_DIR}/Athena/ThirdParty")

set(ASSIMP_INCLUDE_DIR "${THIRD_PARTY_DIR}/assimp/include")
set(BOX2D_INCLUDE_DIR "${THIRD_PARTY_DIR}/Box2D/include")
set(ENTT_INCLUDE_DIR "${THIRD_PARTY_DIR}/entt")
set(GLFW_INCLUDE_DIR "${THIRD_PARTY_DIR}/GLFW/include")
set(IMGUI_INCLUDE_DIR "${THIRD_PARTY_DIR}/ImGui")
set(IMGUIZMO_INCLUDE_DIR "${THIRD_PARTY_DIR}/ImGuizmo")
set(OPTICK_INCLUDE_DIR "${THIRD_PARTY_DIR}/Optick/OptickCore/src")
set(SPDLOG_INCLUDE_DIR "${THIRD_PARTY_DIR}/spdlog/include")
set(STBIMAGE_INCLUDE_DIR "${THIRD_PARTY_DIR}/stb_image/")
set(YAML_CPP_INCLUDE_DIR "${THIRD_PARTY_DIR}/yaml-cpp/include")
set(MSDF_ATLAS_GEN_INCLUDE_DIR "${THIRD_PARTY_DIR}/msdf-atlas-gen")
set(MSDF_GEN_INCLUDE_DIR "${THIRD_PARTY_DIR}/msdf-atlas-gen/msdfgen")

target_include_directories(${PROJECT_NAME} PRIVATE 
	"${ATHENA_SOURCE_DIR}/Athena/Source" 
	${THIRD_PARTY_DIR}
	${ENTT_INCLUDE_DIR}
	${IMGUI_INCLUDE_DIR}
	${IMGUIZMO_INCLUDE_DIR}
	${OPTICK_INCLUDE_DIR}
	${SPDLOG_INCLUDE_DIR}
	${MSDF_ATLAS_GEN_INCLUDE_DIR}
	${MSDF_GEN_INCLUDE_DIR}
)

# Find Vulkan
find_package(Vulkan COMPONENTS shaderc_combined SPIRV-Tools REQUIRED)
if(Vulkan_FOUND)
    set(VULKAN_SDK_PATH "$ENV{VULKAN_SDK}")
    message("-- Find VulkanSDK - ${VULKAN_SDK_PATH}")
else(Vulkan_FOUND)
	message("-- Failed to find VulkanSDK(Install VulkanSDK from official website or check if it is added to path)")
endif(Vulkan_FOUND)

# Show shaders in solution explorer
file(GLOB_RECURSE SHADER_FILES CONFIGURE_DEPENDS 
	"${SHADERS_FOLDER}/*.glsl"
    "${SHADERS_FOLDER}/*.glslh"
    "${SHADERS_FOLDER}/*.hlsl"
    "${SHADERS_FOLDER}/*.hlsli")

add_library(Shaders ${SHADER_FILES})

source_group(TREE "${SHADERS_FOLDER}" FILES ${SHADER_FILES} )
set_property(TARGET Shaders PROPERTY LINKER_LANGUAGE CXX)
set_property(TARGET Shaders PROPERTY FOLDER Core)


macro(CopyDLLToBinaries DLL_PATH TARGET_NAME)
    configure_file("${DLL_PATH}" "${BUILD_FOLDER}/Athena/Debug/${TARGET_NAME}" COPYONLY)
    configure_file("${DLL_PATH}" "${BUILD_FOLDER}/Athena/Release/${TARGET_NAME}" COPYONLY)
endmacro()

# copy assimp dlls
if(WIN32)  
    CopyDLLToBinaries("${THIRD_PARTY_DIR}/assimp/libs/Windows/assimp-vc143-mt.dll" "assimp-vc143-mt.dll")
	target_compile_definitions(${PROJECT_NAME} PUBLIC ATN_PLATFORM_WINDOWS UNICODE)
endif(WIN32)

target_link_libraries(${PROJECT_NAME} PRIVATE "${ATHENA_BINARY_DIR}/$<CONFIGURATION>/Athena.lib")

add_subdirectory("${ATHENA_SOURCE_DIR}/Athena" "${CMAKE_SOURCE_DIR}/Build/Projects/Athena")
