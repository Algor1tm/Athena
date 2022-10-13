cmake_minimum_required (VERSION 3.16)

project(glad LANGUAGES C)

set(GLAD_SRC_FILES
	"src/glad.c"
	"include/glad/glad.h"
	"include/KHR/khrplatform.h")

add_library(glad ${GLAD_SRC_FILES})

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${GLAD_SRC_FILES} )

target_include_directories(glad PUBLIC "include")
