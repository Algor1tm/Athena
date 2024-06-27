
cmake_minimum_required(VERSION 3.15)
include(cmake/version.cmake)

option(MSDFGEN_USE_CPP11 "Build with C++11 enabled" ON)
option(BUILD_SHARED_LIBS "Generate dynamic library files instead of static" OFF)


if(BUILD_SHARED_LIBS)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

project(msdf-atlas-gen VERSION ${MSDF_ATLAS_VERSION} LANGUAGES CXX)

add_subdirectory("msdfgen")

find_package(Threads REQUIRED)

file(GLOB_RECURSE MSDF_ATLAS_SRC_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} 
    "msdf-atlas-gen/*.h" 
    "msdf-atlas-gen/*.hpp"
    "msdf-atlas-gen/*.cpp"
)

add_library(msdf-atlas-gen ${MSDF_ATLAS_SRC_FILES})

target_compile_definitions(msdf-atlas-gen PRIVATE
    MSDF_ATLAS_VERSION=${MSDF_ATLAS_VERSION}
    MSDF_ATLAS_VERSION_MAJOR=${MSDF_ATLAS_VERSION_MAJOR}
    MSDF_ATLAS_VERSION_MINOR=${MSDF_ATLAS_VERSION_MINOR}
    MSDF_ATLAS_VERSION_REVISION=${MSDF_ATLAS_VERSION_REVISION}
    MSDF_ATLAS_COPYRIGHT_YEAR=${MSDF_ATLAS_COPYRIGHT_YEAR}
    _CRT_SECURE_NO_WARNINGS
)

target_include_directories(msdf-atlas-gen PRIVATE
    "msdf-atlas-gen"
    "msdfgen"
    "msdfgen/include"
)

if(MSDF_ATLAS_NO_ARTERY_FONT)
    target_compile_definitions(msdf-atlas-gen PRIVATE MSDF_ATLAS_NO_ARTERY_FONT)
else()
    target_include_directories(msdf-atlas-gen PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/artery-font-format)
endif()

target_compile_features(msdf-atlas-gen PRIVATE cxx_std_11)

target_link_libraries(msdf-atlas-gen PRIVATE Threads::Threads)
target_link_libraries(msdf-atlas-gen PRIVATE msdfgen)

if(MSDFGEN_USE_CPP11)
    target_compile_definitions(msdf-atlas-gen PRIVATE MSDFGEN_USE_CPP11)
endif()

if(BUILD_SHARED_LIBS AND WIN32)
    target_compile_definitions(msdf-atlas-gen PRIVATE "MSDF_ATLAS_PUBLIC=__declspec(dllexport)")
    target_compile_definitions(msdf-atlas-gen PRIVATE "MSDF_ATLAS_PUBLIC=__declspec(dllimport)")
else()
    target_compile_definitions(msdf-atlas-gen PRIVATE MSDF_ATLAS_PUBLIC=)
endif()
