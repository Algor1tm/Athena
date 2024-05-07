
cmake_minimum_required(VERSION 3.15)
include(cmake/version.cmake)

option(MSDFGEN_USE_CPP11 "Build with C++11 enabled" ON)
option(BUILD_SHARED_LIBS "Generate dynamic library files instead of static" OFF)

project(msdfgen VERSION ${MSDFGEN_VERSION} LANGUAGES CXX)


if(BUILD_SHARED_LIBS)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

file(GLOB_RECURSE MSDFGEN_SRC_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} 
    "core/*.h" 
    "core/*.hpp"
    "core/*.cpp"
    "ext/*.cpp" 
    "ext/*.h" 
    "ext/*.hpp"
    "include/*.h"
    "lib/*.cpp")

add_library(msdfgen "${CMAKE_CURRENT_SOURCE_DIR}/msdfgen.h" ${MSDFGEN_SRC_FILES})

target_compile_definitions(msdfgen PRIVATE
    MSDFGEN_VERSION=${MSDFGEN_VERSION}
    MSDFGEN_VERSION_MAJOR=${MSDFGEN_VERSION_MAJOR}
    MSDFGEN_VERSION_MINOR=${MSDFGEN_VERSION_MINOR}
    MSDFGEN_VERSION_REVISION=${MSDFGEN_VERSION_REVISION}
    MSDFGEN_COPYRIGHT_YEAR=${MSDFGEN_COPYRIGHT_YEAR}
)

target_include_directories(msdfgen PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "freetype/include"
    "include"
)

if(MSDFGEN_USE_CPP11)
    target_compile_features(msdfgen PRIVATE cxx_std_11)
    target_compile_definitions(msdfgen PRIVATE MSDFGEN_USE_CPP11)
endif()

if(BUILD_SHARED_LIBS AND WIN32)
    target_compile_definitions(msdfgen PRIVATE "MSDFGEN_PUBLIC=__declspec(dllexport)")
    target_compile_definitions(msdfgen PRIVATE "MSDFGEN_PUBLIC=__declspec(dllimport)")
else()
    target_compile_definitions(msdfgen PUBLIC MSDFGEN_PUBLIC=)
endif()

add_subdirectory("freetype")

target_link_libraries(msdfgen PRIVATE freetype)
