cmake_minimum_required(VERSION 3.16)
project(theword C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# ── Raylib ─────────────────────────────────────────────────────────────────
if(EMSCRIPTEN OR ANDROID)
    set(GLFW_BUILD_X11 OFF CACHE BOOL "" FORCE)
    set(GLFW_USE_WAYLAND OFF CACHE BOOL "" FORCE)
endif()

if(ANDROID)
    set(PLATFORM_ANDROID ON CACHE BOOL "" FORCE)
    set(PLATFORM_OS ANDROID)
endif()

FetchContent_Declare(
    raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/5.0.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

set(BUILD_GAMES OFF)
set(BUILD_EXAMPLES OFF)

FetchContent_MakeAvailable(raylib)

# ── Doctest (header-only unit testing) ────────────────────────────────────
FetchContent_Declare(
    doctest
    URL https://github.com/doctest/doctest/archive/refs/tags/v2.4.11.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

FetchContent_MakeAvailable(doctest)

# ── SQLite amalgamation (bundled, no system dependency) ──────────────────
FetchContent_Declare(
    sqlite3
    URL https://www.sqlite.org/2026/sqlite-amalgamation-3530200.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

FetchContent_GetProperties(sqlite3)
if(NOT sqlite3_POPULATED)
    FetchContent_Populate(sqlite3)
endif()

# ── Source files (explicit list — no GLOB_RECURSE) ────────────────────────
set(CORE_BASE_SOURCES
    src/core/Logger.cpp
    src/core/EnvLoader.cpp
    src/core/FontHelper.cpp
    src/core/FileAssetProvider.cpp
)

if(EMSCRIPTEN)
    set(CORE_SOURCES
        src/core/EmscriptenClient.cpp
        ${CORE_BASE_SOURCES}
    )
elseif(ANDROID)
    set(CORE_SOURCES
        src/core/AndroidClient.cpp
        src/core/AndroidAssetProvider.cpp
        ${CORE_BASE_SOURCES}
    )
else()
    set(CORE_SOURCES
        src/core/CurlHttpClient.cpp
        ${CORE_BASE_SOURCES}
    )
endif()

set(DATA_SOURCES
    src/data/BibleClient.cpp
    src/data/CompositeProvider.cpp
    src/data/StubChapterProvider.cpp
    src/data/USFMParser.cpp
)

set(TEXT_SOURCES
    src/text/LayoutEngine.cpp
)

set(DOCUMENT_SOURCES
    src/document/DocumentManager.cpp
)

set(HIGHLIGHT_SOURCES
    src/highlight/Highlighter.cpp
    src/highlight/InMemoryStorage.cpp
)

set(INPUT_SOURCES
    src/input/InputHandler.cpp
)

set(RENDERER_SOURCES
    src/renderer/Renderer.cpp
    src/renderer/UIManager.cpp
)

# ── SQLite object library (compiled once, linked to both targets) ─────────
add_library(sqlite3_obj OBJECT ${sqlite3_SOURCE_DIR}/sqlite3.c)
target_include_directories(sqlite3_obj PRIVATE ${sqlite3_SOURCE_DIR})

set(PERSISTENCE_SOURCES
    src/persistence/PersistenceManager.cpp
)

add_executable(theword
    src/main.cpp
    ${CORE_SOURCES}
    ${DATA_SOURCES}
    ${TEXT_SOURCES}
    ${DOCUMENT_SOURCES}
    ${HIGHLIGHT_SOURCES}
    ${INPUT_SOURCES}
    ${PERSISTENCE_SOURCES}
    ${RENDERER_SOURCES}
    $<TARGET_OBJECTS:sqlite3_obj>
)

target_include_directories(theword PRIVATE src ${sqlite3_SOURCE_DIR})

if(NOT EMSCRIPTEN AND NOT ANDROID)
    add_executable(theword_test
        tests/test_main.cpp
        ${CORE_SOURCES}
        ${DATA_SOURCES}
        ${TEXT_SOURCES}
        ${DOCUMENT_SOURCES}
        ${HIGHLIGHT_SOURCES}
        ${INPUT_SOURCES}
        ${PERSISTENCE_SOURCES}
        ${RENDERER_SOURCES}
        $<TARGET_OBJECTS:sqlite3_obj>
    )
    target_include_directories(theword_test PRIVATE src ${doctest_SOURCE_DIR} ${sqlite3_SOURCE_DIR})
endif()

# ── Asset copying ─────────────────────────────────────────────────────────
file(COPY assets DESTINATION ${CMAKE_BINARY_DIR})
file(COPY shaders DESTINATION ${CMAKE_BINARY_DIR})

# ── Linking ───────────────────────────────────────────────────────────────
if(EMSCRIPTEN)
    target_link_options(theword PRIVATE
        "-sUSE_GLFW=3"
        "-sASYNCIFY=1"
        "-sTOTAL_MEMORY=64MB"
        "-sMAX_WEBGL_VERSION=2"
        "-sFORCE_FILESYSTEM=1"
        "-sFETCH=1"
        "--preload-file" "assets"
        "--shell-file" "${CMAKE_SOURCE_DIR}/wasm_shell.html"
    )
    set_target_properties(theword PROPERTIES SUFFIX ".html")
    target_link_libraries(theword PRIVATE raylib)
elseif(ANDROID)
    # Bundle libcurl for Android NDK
    FetchContent_Declare(
        curl
        URL https://curl.se/download/curl-8.4.0.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    set(CURL_USE_OPENSSL OFF CACHE BOOL "" FORCE)
    set(CURL_DISABLE_CRYPTO_AUTH ON CACHE BOOL "" FORCE)
    set(ENABLE_ARES OFF CACHE BOOL "" FORCE)
    set(BUILD_CURL_EXE OFF CACHE BOOL "" FORCE)
    set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
    set(HTTP_ONLY ON CACHE BOOL "" FORCE)
    FetchContent_GetProperties(curl)
    if(NOT curl_POPULATED)
        FetchContent_Populate(curl)
    endif()

    # Build libcurl as a static library for Android NDK
    add_subdirectory(${curl_SOURCE_DIR} ${curl_BINARY_DIR} EXCLUDE_FROM_ALL)

    target_compile_definitions(theword PRIVATE CURL_STATICLIB)
    target_include_directories(theword PRIVATE
        ${curl_SOURCE_DIR}/include
        ${ANDROID_NDK}/sources/android/native_app_glue
    )
    target_link_libraries(theword PRIVATE
        raylib
        libcurl_static
        android
        EGL
        GLESv2
        log
    )
    target_link_options(theword PRIVATE
        "-uANativeActivity_onCreate"
        "-Wl,-undefined,dynamic_lookup"
    )
    set_target_properties(theword PROPERTIES
        SUFFIX ".so"
        PREFIX "lib"
    )
elseif(WIN32)
    target_link_libraries(theword PRIVATE
        raylib
        opengl32
        gdi32
        winmm
    )
    target_link_libraries(theword_test PRIVATE
        raylib
        opengl32
        gdi32
        winmm
    )
else()
    find_package(CURL REQUIRED)
    target_link_libraries(theword PRIVATE
        raylib
        ${CURL_LIBRARIES}
        m
        pthread
        dl
        rt
        X11
        Xcursor
        Xrandr
        Xi
    )
    target_link_libraries(theword_test PRIVATE
        raylib
        ${CURL_LIBRARIES}
        m
        pthread
        dl
        rt
        X11
        Xcursor
        Xrandr
        Xi
    )
endif()
