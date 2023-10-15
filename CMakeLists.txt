cmake_minimum_required (VERSION 3.24)

set(BUILD_FOLDER "${CMAKE_SOURCE_DIR}/Build/Binaries")
set(SCRIPTS_FOLDER "${CMAKE_SOURCE_DIR}/Athena-Editor/Assets/Scripts")
set(SHADERS_FOLDER "${CMAKE_SOURCE_DIR}/Athena/EngineResources/ShaderPack")

# Enable in cross-platform way multi-processor compilation
macro(EnableMultiProcessorCompilation TARGET_NAME)
    if(MSVC)
	    target_compile_options(${TARGET_NAME} PUBLIC /MP)
    endif(MSVC)
    #TODO: support other compilers
endmacro()

macro(CopyDLLToBinaries DLL_PATH TARGET_NAME)
    configure_file("${DLL_PATH}" "${BUILD_FOLDER}/Athena/Debug/${TARGET_NAME}" COPYONLY)
    configure_file("${DLL_PATH}" "${BUILD_FOLDER}/Athena/Release/${TARGET_NAME}" COPYONLY)
    configure_file("${DLL_PATH}" "${BUILD_FOLDER}/Athena-Editor/Debug/${TARGET_NAME}" COPYONLY)
    configure_file("${DLL_PATH}" "${BUILD_FOLDER}/Athena-Editor/Release/${TARGET_NAME}" COPYONLY)
    configure_file("${DLL_PATH}" "${BUILD_FOLDER}/SandBox/Debug/${TARGET_NAME}" COPYONLY)
    configure_file("${DLL_PATH}" "${BUILD_FOLDER}/SandBox/Release/${TARGET_NAME}" COPYONLY)
endmacro()


project ("Athena")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT "SandBox")

set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)

# Find Vulkan
find_package(Vulkan COMPONENTS shaderc_combined SPIRV-Tools REQUIRED)
if(Vulkan_FOUND)
    set(VULKAN_SDK_PATH "$ENV{VULKAN_SDK}")
    message("-- Find VulkanSDK - ${VULKAN_SDK_PATH}")
else(Vulkan_FOUND)
	message("-- Failed to find VulkanSDK(Install VulkanSDK from official website or check if it is added to path)")
endif(Vulkan_FOUND)

# Find Python 3.6+
find_package(Python 3.6 COMPONENTS Interpreter Development REQUIRED)

get_target_property(Lib Python::Python IMPORTED_IMPLIB)
message(STATUS "Python3 - ${Lib}")

get_target_property(LibDebug Python::Python IMPORTED_IMPLIB_DEBUG)
message(STATUS "Python3 (Debug) - ${LibDebug}")

if(LibDebug)
    set(ATN_PY_DEBUG 1)
endif(LibDebug)

get_target_property(LibRelease Python::Python IMPORTED_IMPLIB_RELEASE)
message(STATUS "Python3 (Release) - ${LibRelease}")

# Show python scripts in solution explorer
file(GLOB_RECURSE PYTHON_FILES CONFIGURE_DEPENDS 
	"${SCRIPTS_FOLDER}/*.py")

add_library(PythonScripts ${PYTHON_FILES})

source_group(TREE "${SCRIPTS_FOLDER}" FILES ${PYTHON_FILES} )
set_property(TARGET PythonScripts PROPERTY LINKER_LANGUAGE CXX)
set_property(TARGET PythonScripts PROPERTY FOLDER Core)

# Show shaders in solution explorer
file(GLOB_RECURSE SHADER_FILES CONFIGURE_DEPENDS 
	"${SHADERS_FOLDER}/*.glsl"
    "${SHADERS_FOLDER}/*.glsli"
    "${SHADERS_FOLDER}/*.hlsl"
    "${SHADERS_FOLDER}/*.hlsli")

add_library(Shaders ${SHADER_FILES})

source_group(TREE "${SHADERS_FOLDER}" FILES ${SHADER_FILES} )
set_property(TARGET Shaders PROPERTY LINKER_LANGUAGE CXX)
set_property(TARGET Shaders PROPERTY FOLDER Core)


# copy assimp dlls
if(WIN32)  
    CopyDLLToBinaries("${CMAKE_SOURCE_DIR}/Athena/ThirdParty/assimp/libs/Windows/assimp-vc143-mt.dll" "assimp-vc143-mt.dll")
endif(WIN32)

include(Dependencies.cmake)

add_subdirectory("Athena")
add_subdirectory("Athena-Editor")
add_subdirectory("SandBox")
