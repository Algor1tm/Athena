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
set(BUILD_FOLDER "${CMAKE_SOURCE_DIR}/bin")

include(Dependencies.cmake)

# ThirdParty
add_subdirectory("${THIRD_PARTY_DIR}/Box2D")
add_subdirectory("${THIRD_PARTY_DIR}/glad")
add_subdirectory("${THIRD_PARTY_DIR}/GLFW")
add_subdirectory("${THIRD_PARTY_DIR}/ImGui")
add_subdirectory("${THIRD_PARTY_DIR}/spdlog")
add_subdirectory("${THIRD_PARTY_DIR}/yaml-cpp")

add_subdirectory("Athena")
add_subdirectory("Athena-Editor")
add_subdirectory("SandBox")
