set(TRACY_UTIL_DIR ${CMAKE_CURRENT_LIST_DIR}/../util)

set(TRACY_UTIL_SOURCES
    CaptureConsole.cpp
    CaptureFileBackup.cpp
    CaptureOutput.cpp
)

list(TRANSFORM TRACY_UTIL_SOURCES PREPEND "${TRACY_UTIL_DIR}/")

add_library(TracyUtil STATIC EXCLUDE_FROM_ALL ${TRACY_UTIL_SOURCES})
target_link_libraries(TracyUtil PUBLIC TracyServer)
target_include_directories(TracyUtil PUBLIC ${TRACY_UTIL_DIR})
