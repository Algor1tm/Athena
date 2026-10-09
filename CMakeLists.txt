cmake_minimum_required (VERSION 3.24)

######################################
##### CONFIG	
######################################

set(BUILD_FOLDER "${CMAKE_SOURCE_DIR}/Build/Binaries")
set(SHADERS_FOLDER "${CMAKE_SOURCE_DIR}/Athena/EngineResources/ShaderPack")

######################################
##### GLOBAL
######################################

project ("Athena")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT "Athena-Editor")

set(CMAKE_CONFIGURATION_TYPES "Debug;Release;Dist" CACHE STRING "" FORCE)

set(CMAKE_CXX_FLAGS_DIST "${CMAKE_CXX_FLAGS_RELEASE}")
set(CMAKE_C_FLAGS_DIST "${CMAKE_C_FLAGS_RELEASE}")
set(CMAKE_SHARED_LINKER_FLAGS_DIST "${CMAKE_SHARED_LINKER_FLAGS_RELEASE}")
set(CMAKE_EXE_LINKER_FLAGS_DIST "${CMAKE_EXE_LINKER_FLAGS_RELEASE}")

# Force to generate debug info in Release mode
if(MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /Zi")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /O2 /Zi")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG")
else(MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -g")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -g")
endif(MSVC)

# Find Vulkan
find_package(Vulkan COMPONENTS shaderc_combined SPIRV-Tools REQUIRED)
if(Vulkan_FOUND)
    set(VULKAN_SDK_PATH "$ENV{VULKAN_SDK}")
    message("-- Find VulkanSDK - ${VULKAN_SDK_PATH}")
else(Vulkan_FOUND)
	message("-- Failed to find VulkanSDK(Install VulkanSDK from official website or check if it is added to path)")
endif(Vulkan_FOUND)

######################################
##### SHADERS PROJECT
######################################

file(GLOB_RECURSE SHADER_FILES CONFIGURE_DEPENDS 
	"${SHADERS_FOLDER}/*.glsl"
    "${SHADERS_FOLDER}/*.glslh"
    "${SHADERS_FOLDER}/*.hlsl"
    "${SHADERS_FOLDER}/*.hlsli"
)

add_library(Shaders ${SHADER_FILES})

source_group(TREE "${SHADERS_FOLDER}" FILES ${SHADER_FILES} )
set_target_properties(Shaders
	PROPERTIES
	LINKER_LANGUAGE CXX
	FOLDER Core
)

######################################
##### DEPENDENCIES
######################################

# copy assimp dlls
macro(CopyDLLToBinaries DLL_PATH TARGET_NAME)
    configure_file("${DLL_PATH}" "${BUILD_FOLDER}/Athena/Debug/${TARGET_NAME}" COPYONLY)
    configure_file("${DLL_PATH}" "${BUILD_FOLDER}/Athena/Release/${TARGET_NAME}" COPYONLY)
    configure_file("${DLL_PATH}" "${BUILD_FOLDER}/Athena/Dist/${TARGET_NAME}" COPYONLY)
    configure_file("${DLL_PATH}" "${BUILD_FOLDER}/Athena-Editor/Debug/${TARGET_NAME}" COPYONLY)
    configure_file("${DLL_PATH}" "${BUILD_FOLDER}/Athena-Editor/Release/${TARGET_NAME}" COPYONLY)
    configure_file("${DLL_PATH}" "${BUILD_FOLDER}/Athena-Editor/Dist/${TARGET_NAME}" COPYONLY)
    configure_file("${DLL_PATH}" "${BUILD_FOLDER}/SandBox/Debug/${TARGET_NAME}" COPYONLY)
    configure_file("${DLL_PATH}" "${BUILD_FOLDER}/SandBox/Release/${TARGET_NAME}" COPYONLY)
    configure_file("${DLL_PATH}" "${BUILD_FOLDER}/SandBox/Dist/${TARGET_NAME}" COPYONLY)
endmacro()

if(WIN32)  
    CopyDLLToBinaries("${CMAKE_SOURCE_DIR}/Athena/ThirdParty/assimp/libs/Windows/assimp-vc143-mt.dll" "assimp-vc143-mt.dll")
endif(WIN32)

set(THIRD_PARTY_DIR "${CMAKE_SOURCE_DIR}/Athena/ThirdParty")
include(Dependencies.cmake)

add_subdirectory("Athena")
add_subdirectory("Athena-Editor")
add_subdirectory("SandBox")
