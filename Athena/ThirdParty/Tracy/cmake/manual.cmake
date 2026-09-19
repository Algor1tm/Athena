set(PANDOC_VERSION 3.11)

find_package(Python3 COMPONENTS Interpreter REQUIRED)

find_program(TRACY_PANDOC pandoc)
if(TRACY_PANDOC)
    execute_process(
        COMMAND ${TRACY_PANDOC} --version
        OUTPUT_VARIABLE _pandoc_version
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE _pandoc_rc
        ERROR_QUIET)
    string(REGEX MATCH "pandoc [0-9.]+" _pandoc_version "${_pandoc_version}")
    if(NOT _pandoc_rc EQUAL 0 OR NOT _pandoc_version STREQUAL "pandoc ${PANDOC_VERSION}")
        set(TRACY_PANDOC "")
    endif()
endif()
if(TRACY_PANDOC)
    message(STATUS "Using system pandoc ${PANDOC_VERSION}: ${TRACY_PANDOC}")
else()
    if(WIN32)
        if(NOT CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|amd64)$")
            message(FATAL_ERROR "No pinned pandoc ${PANDOC_VERSION} binary for ${CMAKE_HOST_SYSTEM_PROCESSOR}; install pandoc ${PANDOC_VERSION}")
        endif()
        set(_pandoc_asset pandoc-${PANDOC_VERSION}-windows-x86_64.zip)
        set(_pandoc_hash 2ab72baf2399450e148ddf7a2a8689806c42e1bba71862b57e220fd9b8456d3d)
        set(_pandoc_exe pandoc-${PANDOC_VERSION}/pandoc.exe)
    elseif(APPLE)
        if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "^(arm64|aarch64)$")
            set(_pandoc_asset pandoc-${PANDOC_VERSION}-arm64-macOS.zip)
            set(_pandoc_hash 15806bedf9517bfead72e88fe6a6696635c3691efbb6e152173440e9c5bb50b4)
            set(_pandoc_exe pandoc-${PANDOC_VERSION}-arm64/bin/pandoc)
        elseif(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|amd64)$")
            set(_pandoc_asset pandoc-${PANDOC_VERSION}-x86_64-macOS.zip)
            set(_pandoc_hash 3b1c1b57f160112c821d02f23d946ede8b7f57a6ccf4632a25a512d334a9291f)
            set(_pandoc_exe pandoc-${PANDOC_VERSION}-x86_64/bin/pandoc)
        else()
            message(FATAL_ERROR "No pinned pandoc ${PANDOC_VERSION} binary for ${CMAKE_HOST_SYSTEM_PROCESSOR}; install pandoc ${PANDOC_VERSION}")
        endif()
    else()
        if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)$")
            set(_pandoc_asset pandoc-${PANDOC_VERSION}-linux-arm64.tar.gz)
            set(_pandoc_hash 56ed5566ec41d22ec9ee0704e6ac0b98ba102e92384efd5306173a22d314c79a)
            set(_pandoc_exe pandoc-${PANDOC_VERSION}/bin/pandoc)
        elseif(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|amd64)$")
            set(_pandoc_asset pandoc-${PANDOC_VERSION}-linux-amd64.tar.gz)
            set(_pandoc_hash 37edb3bbcf722f921a009941bf5874e2e0c09263226c9b4a2d980788cb062ab6)
        set(_pandoc_exe pandoc-${PANDOC_VERSION}/bin/pandoc)
        else()
            message(FATAL_ERROR "No pinned pandoc ${PANDOC_VERSION} binary for ${CMAKE_HOST_SYSTEM_PROCESSOR}; install pandoc ${PANDOC_VERSION}")
        endif()
    endif()

    # Shared cache: $PANDOC_CACHE_DIR, else the platform user cache directory.
    if(DEFINED ENV{PANDOC_CACHE_DIR})
        set(_pandoc_cache $ENV{PANDOC_CACHE_DIR})
    elseif(WIN32)
        set(_pandoc_cache $ENV{LOCALAPPDATA}/tracy/pandoc)
    elseif(APPLE)
        set(_pandoc_cache $ENV{HOME}/Library/Caches/tracy/pandoc)
    elseif(DEFINED ENV{XDG_CACHE_HOME})
        set(_pandoc_cache $ENV{XDG_CACHE_HOME}/tracy/pandoc)
    else()
        set(_pandoc_cache $ENV{HOME}/.cache/tracy/pandoc)
    endif()

    set(TRACY_PANDOC ${_pandoc_cache}/${_pandoc_exe})
    if(NOT EXISTS ${TRACY_PANDOC})
        # Concurrent configures on a cold cache can collide; worst case the
        # hash check fails and a rerun succeeds.
        message(STATUS "Downloading pandoc ${PANDOC_VERSION} (${_pandoc_asset})")
        file(MAKE_DIRECTORY ${_pandoc_cache})
        file(DOWNLOAD
            https://github.com/jgm/pandoc/releases/download/${PANDOC_VERSION}/${_pandoc_asset}
            ${_pandoc_cache}/pandoc.archive
            EXPECTED_HASH SHA256=${_pandoc_hash}
            STATUS _pandoc_download)
        list(GET _pandoc_download 0 _pandoc_download_rc)
        if(NOT _pandoc_download_rc STREQUAL "0")
            file(REMOVE ${_pandoc_cache}/pandoc.archive)
            message(FATAL_ERROR "Failed to download pandoc ${PANDOC_VERSION}: ${_pandoc_download}")
        endif()
        file(ARCHIVE_EXTRACT
            INPUT ${_pandoc_cache}/pandoc.archive
            DESTINATION ${_pandoc_cache})
        file(REMOVE ${_pandoc_cache}/pandoc.archive)
    endif()
endif()
