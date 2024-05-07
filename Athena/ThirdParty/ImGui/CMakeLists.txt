cmake_minimum_required (VERSION 3.16)

project(ImGui)

set(IMGUI_SRC_FILES
    "imconfig.h"
    "imgui.cpp"
    "imgui.h"
    "imgui_demo.cpp"
    "imgui_draw.cpp"
    "imgui_internal.h"
    "imgui_tables.cpp"
    "imgui_widgets.cpp"
    "imstb_rectpack.h"
    "imstb_textedit.h"
    "imstb_truetype.h")

add_library(ImGui ${IMGUI_SRC_FILES})

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${IMGUI_SRC_FILES} )

set_target_properties(ImGui PROPERTIES CXX_STANDARD 17)
