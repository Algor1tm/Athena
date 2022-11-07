cmake_minimum_required (VERSION 3.16)

# Enable in cross-platform way multi-processor compilation
macro(EnableMultiProcessorCompilation TARGET_NAME)
    if(MSVC)
	    target_compile_options(${TARGET_NAME} PUBLIC /MP)
    endif(MSVC)
    #TODO: support other compilers
endmacro()


project ("Athena")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT "Athena-Editor")

set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)
set(BUILD_FOLDER "${CMAKE_SOURCE_DIR}/Build/Binaries")
set(SCRIPTS_FOLDER "${CMAKE_SOURCE_DIR}/Athena-Editor/Assets/Scripts")

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

# Show python scripts in explorer
file(GLOB_RECURSE PYTHON_FILES CONFIGURE_DEPENDS 
	"${SCRIPTS_FOLDER}/*.py")

add_library(Athena-RuntimeScripts ${PYTHON_FILES})

source_group(TREE "${SCRIPTS_FOLDER}/../" FILES ${PYTHON_FILES} )
set_property(TARGET Athena-RuntimeScripts PROPERTY LINKER_LANGUAGE CXX)
set_property(TARGET Athena-RuntimeScripts PROPERTY FOLDER Core)

include(Dependencies.cmake)

# ThirdParty
add_subdirectory("${THIRD_PARTY_DIR}/Box2D")
add_subdirectory("${THIRD_PARTY_DIR}/glad")
add_subdirectory("${THIRD_PARTY_DIR}/GLFW")
add_subdirectory("${THIRD_PARTY_DIR}/ImGui")
add_subdirectory("${THIRD_PARTY_DIR}/pybind11")
add_subdirectory("${THIRD_PARTY_DIR}/spdlog")
add_subdirectory("${THIRD_PARTY_DIR}/yaml-cpp")

add_subdirectory("Athena")
add_subdirectory("Athena-Editor")
add_subdirectory("SandBox")
