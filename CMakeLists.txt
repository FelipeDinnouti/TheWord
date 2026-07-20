cmake_minimum_required(VERSION 3.16)

# Suppress deprecation warning when using FetchContent_Populate() for declared
# dependencies (needed for the raylib Android patch workflow). We use Populate
# instead of MakeAvailable so we can apply a patch before add_subdirectory.
# Policy available in CMake >= 3.25; ignored silently on older versions.
if(POLICY CMP0169)
    cmake_policy(SET CMP0169 OLD)
endif()

project(theword VERSION 1.8.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Pre-release suffix (empty for release builds, e.g. "-alpha.1")
set(THEWORD_VERSION_SUFFIX "")
set(APP_VERSION_FULL "${PROJECT_VERSION}${THEWORD_VERSION_SUFFIX}")

# Auto-generate Version.h from template (substitutes @PROJECT_VERSION@ etc.)
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/core/Version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/Version.h"
    @ONLY
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

# ── Test Monitor tool ──────────────────────────────────────────────────────
option(BUILD_TEST_MONITOR "Build the test monitor (separate window showing test steps)" OFF)
if(BUILD_TEST_MONITOR)
    add_subdirectory(tools/test_monitor)
endif()

# ── Raylib ─────────────────────────────────────────────────────────────────
if(EMSCRIPTEN OR ANDROID)
    set(GLFW_BUILD_X11 OFF CACHE BOOL "" FORCE)
    set(GLFW_USE_WAYLAND OFF CACHE BOOL "" FORCE)
endif()

if(ANDROID)
    set(PLATFORM "Android" CACHE STRING "Platform to build for." FORCE)
    if(NOT ANDROID_NDK AND CMAKE_ANDROID_NDK)
        set(ANDROID_NDK "${CMAKE_ANDROID_NDK}" CACHE STRING "Android NDK path" FORCE)
    endif()
endif()

FetchContent_Declare(
    raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/5.0.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

set(BUILD_GAMES OFF)
set(BUILD_EXAMPLES OFF)

# ── Android: apply patch for GetCharPressed + VSYNC ──────────────────────
# raylib 5.0's rcore_android.c has several unfixed issues:
#   1. Gamepad source flag collision (#5387) — keyboard events with
#      AINPUT_SOURCE_GAMEPAD bit set get swallowed by the gamepad handler.
#   2. charPressedQueue is never populated — GetCharPressed() returns 0
#      because there is no character callback like GLFW's CharCallback.
#   3. eglSwapInterval() is commented out in InitGraphicsDevice() — VSYNC
#      never enabled on Android even with FLAG_VSYNC_HINT set.
#   4. eglSwapInterval() missing from the contextRebind path — VSYNC lost
#      on activity resume after surface re-creation.
#
# The patch fixes all four and adds JNI-based Unicode character capture.
# See cmake/patches/raylib-5.0-android-char-input.patch for details.
if(ANDROID)
    FetchContent_GetProperties(raylib)
    if(NOT raylib_POPULATED)
        FetchContent_Populate(raylib)
        find_program(PATCH_PROGRAM patch REQUIRED)
        execute_process(
            COMMAND ${PATCH_PROGRAM} -p1 -N -i
                ${CMAKE_SOURCE_DIR}/cmake/patches/raylib-5.0-android-char-input.patch
            WORKING_DIRECTORY ${raylib_SOURCE_DIR}
            RESULT_VARIABLE _patch_result
        )
        if(_patch_result EQUAL 0)
            message(STATUS "Applied raylib Android patch: GetCharPressed support + gamepad fix")
        elseif(_patch_result EQUAL 1)
            message(STATUS "Raylib Android patch already applied, skipping")
        else()
            message(FATAL_ERROR "Failed to patch raylib for Android text input support (exit=${_patch_result})")
        endif()
    endif()
    add_subdirectory(${raylib_SOURCE_DIR} ${raylib_BINARY_DIR})
else()
    FetchContent_MakeAvailable(raylib)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(raylib PRIVATE -Wno-implicit-const-int-float-conversion)
endif()

# ── Doctest (header-only unit testing) ────────────────────────────────────
FetchContent_Declare(
    doctest
    URL https://github.com/doctest/doctest/archive/refs/tags/v2.4.11.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

FetchContent_MakeAvailable(doctest)

# ── mbedTLS (Android only — lightweight TLS for curl HTTPS support) ──────
FetchContent_Declare(
    mbedtls
    URL https://github.com/Mbed-TLS/mbedtls/archive/refs/tags/v2.28.9.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

# ── libcurl (optional, for online Bible API) ──────────────────────────────
if(EMSCRIPTEN)
    # No curl on Emscripten
elseif(MINGW)
    # Cross-compile curl from source for Windows (MinGW) with native SSL (Schannel)
    FetchContent_Declare(
        curl
        URL https://curl.se/download/curl-8.4.0.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    set(CURL_USE_SCHANNEL ON CACHE BOOL "" FORCE)
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
    add_subdirectory(${curl_SOURCE_DIR} ${curl_BINARY_DIR} EXCLUDE_FROM_ALL)
    set(CURL_FOUND TRUE)
    set(CURL_INCLUDE_DIRS ${curl_SOURCE_DIR}/include)
elseif(ANDROID)
    # Build mbedTLS (lightweight TLS) for HTTPS support
    set(UNITTEST OFF CACHE BOOL "" FORCE)
    set(ENABLE_PROGRAMS OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(mbedtls)

    # Set mbedTLS paths for curl's FindMbedTLS.cmake to pick up
    set(MBEDTLS_INCLUDE_DIRS "${mbedtls_SOURCE_DIR}/include" CACHE PATH "" FORCE)
    set(MBEDTLS_LIBRARY "${mbedtls_BINARY_DIR}/library/libmbedtls.a" CACHE FILEPATH "" FORCE)
    set(MBEDCRYPTO_LIBRARY "${mbedtls_BINARY_DIR}/library/libmbedcrypto.a" CACHE FILEPATH "" FORCE)
    set(MBEDX509_LIBRARY "${mbedtls_BINARY_DIR}/library/libmbedx509.a" CACHE FILEPATH "" FORCE)

    # Build curl from source for Android NDK, with mbedTLS for HTTPS
    FetchContent_Declare(
        curl
        URL https://curl.se/download/curl-8.4.0.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    set(CURL_USE_OPENSSL OFF CACHE BOOL "" FORCE)
    set(CURL_USE_MBEDTLS ON CACHE BOOL "" FORCE)
    set(CURL_DISABLE_CRYPTO_AUTH ON CACHE BOOL "" FORCE)
    set(ENABLE_ARES OFF CACHE BOOL "" FORCE)
    set(BUILD_CURL_EXE OFF CACHE BOOL "" FORCE)
    set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
    FetchContent_GetProperties(curl)
    if(NOT curl_POPULATED)
        FetchContent_Populate(curl)
    endif()
    add_subdirectory(${curl_SOURCE_DIR} ${curl_BINARY_DIR} EXCLUDE_FROM_ALL)
    set(CURL_FOUND TRUE)
    set(CURL_INCLUDE_DIRS ${curl_SOURCE_DIR}/include)
else()
    find_package(CURL QUIET)
endif()

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

# ── SQLite object library (compiled once, linked to both targets) ─────────
add_library(sqlite3_obj OBJECT ${sqlite3_SOURCE_DIR}/sqlite3.c)
target_include_directories(sqlite3_obj SYSTEM PUBLIC ${sqlite3_SOURCE_DIR})

# ── Source files (explicit list — no GLOB_RECURSE) ────────────────────────
set(EVENT_SOURCES
    src/event/EventBus.h
    src/event/Events.h
)

set(CORE_BASE_SOURCES
    src/core/Logger.cpp
    src/core/Logger.h
    src/core/EnvLoader.cpp
    src/core/EnvLoader.h
    src/core/FontHelper.cpp
    src/core/FontHelper.h
    src/core/FileAssetProvider.cpp
    src/core/FileAssetProvider.h
    src/core/FuzzyMatcher.cpp
    src/core/FuzzyMatcher.h
    src/core/Platform.cpp
    src/core/Platform.h
    src/core/Config.h
    src/core/ThemeManager.cpp
    src/core/ThemeManager.h
    src/core/UIScale.h
    src/core/Locale.cpp
    src/core/Locale.h
    src/core/BibleBooks.h
    src/core/IHttpClient.h
    src/core/IAssetProvider.h
)

if(EMSCRIPTEN)
    set(CORE_SOURCES
        src/core/EmscriptenClient.cpp
        src/core/EmscriptenClient.h
        ${CORE_BASE_SOURCES}
    )
elseif(ANDROID)
    set(CORE_SOURCES
        src/core/CurlHttpClient.cpp
        src/core/CurlHttpClient.h
        src/core/AndroidAssetProvider.cpp
        src/core/AndroidAssetProvider.h
        ${CORE_BASE_SOURCES}
    )
else()
    if(CURL_FOUND)
        set(CORE_SOURCES
            src/core/CurlHttpClient.cpp
            src/core/CurlHttpClient.h
            ${CORE_BASE_SOURCES}
        )
    else()
        set(CORE_SOURCES
            ${CORE_BASE_SOURCES}
        )
    endif()
endif()

set(DATA_SOURCES
    src/data/BibleClient.cpp
    src/data/BibleClient.h
    src/data/ChapterProvider.h
    src/data/CompositeProvider.cpp
    src/data/CompositeProvider.h
    src/data/StubChapterProvider.cpp
    src/data/StubChapterProvider.h
    src/data/USFMParser.cpp
    src/data/USFMParser.h
    src/data/DataUtils.h
)

set(TEXT_SOURCES
    src/text/LayoutEngine.cpp
    src/text/LayoutEngine.h
)

set(DOCUMENT_SOURCES
    src/document/DocumentManager.cpp
    src/document/DocumentManager.h
)

set(HIGHLIGHT_SOURCES
    src/highlight/Highlighter.cpp
    src/highlight/Highlighter.h
    src/highlight/InMemoryStorage.cpp
    src/highlight/InMemoryStorage.h
    src/highlight/PersistenceInterface.h
)

set(INPUT_SOURCES
    src/input/InputHandler.cpp
    src/input/InputHandler.h
)

set(RENDERER_SOURCES
    src/renderer/Renderer.cpp
    src/renderer/Renderer.h
    src/renderer/UIManager.cpp
    src/renderer/UIManager.h
    src/renderer/RadialMenu.cpp
    src/renderer/RadialMenu.h
)

set(UI_SOURCES
    src/ui/Screen.h
    src/ui/NavigationStack.h
    src/ui/NavigationStack.cpp
    src/ui/ReaderScreen.h
    src/ui/ReaderScreen.cpp
    src/ui/CenterMenu.h
    src/ui/CenterMenu.cpp
    src/ui/BookListScreen.h
    src/ui/BookListScreen.cpp
    src/ui/ChapterGridScreen.h
    src/ui/ChapterGridScreen.cpp
    src/ui/components.h
    src/ui/components.cpp
    src/ui/SettingsScreen.h
    src/ui/SettingsScreen.cpp
    src/ui/CreditsOverlay.h
    src/ui/CreditsOverlay.cpp
    src/ui/TapDetector.h
    src/ui/FontDiagnostic.h
    src/ui/FontDiagnostic.cpp
    src/ui/HighlightBrowserScreen.h
    src/ui/HighlightBrowserScreen.cpp
)

set(APP_SOURCES
    src/app/App.h
    src/app/App.cpp
)

set(PERSISTENCE_SOURCES
    src/persistence/PersistenceManager.cpp
    src/persistence/PersistenceManager.h
)

# ── Object library (shared sources compiled once for both targets) ────────
add_library(theword_objects OBJECT
    ${APP_SOURCES}
    ${EVENT_SOURCES}
    ${CORE_SOURCES}
    ${DATA_SOURCES}
    ${TEXT_SOURCES}
    ${DOCUMENT_SOURCES}
    ${HIGHLIGHT_SOURCES}
    ${INPUT_SOURCES}
    ${PERSISTENCE_SOURCES}
    ${RENDERER_SOURCES}
    ${UI_SOURCES}
)


target_include_directories(theword_objects PUBLIC src)
target_include_directories(theword_objects PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")
target_link_libraries(theword_objects PRIVATE raylib sqlite3_obj)

if(EMSCRIPTEN OR ANDROID OR CURL_FOUND)
    target_compile_definitions(theword_objects PRIVATE THEWORD_HAS_HTTP)
endif()

if(NOT MSVC)
    target_compile_options(theword_objects PRIVATE -Wall -Wextra -Wpedantic)
    if(ANDROID OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(theword_objects PRIVATE -Wno-implicit-const-int-float-conversion)
    endif()
endif()

# ── Executables ────────────────────────────────────────────────────────────
if(ANDROID)
    add_library(theword SHARED src/main.cpp $<TARGET_OBJECTS:theword_objects> $<TARGET_OBJECTS:sqlite3_obj>)
else()
    add_executable(theword src/main.cpp $<TARGET_OBJECTS:theword_objects> $<TARGET_OBJECTS:sqlite3_obj>)
endif()

target_include_directories(theword PRIVATE src)

target_compile_definitions(theword PRIVATE THEWORD_VERSION="1.0.0")

if(NOT EMSCRIPTEN AND NOT ANDROID AND NOT WIN32)
    add_executable(theword_test tests/test_main.cpp $<TARGET_OBJECTS:theword_objects> $<TARGET_OBJECTS:sqlite3_obj>)
    target_include_directories(theword_test PRIVATE src ${doctest_SOURCE_DIR})
    target_include_directories(theword_test PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")
endif()

# ── Asset copying (build-time, recopies every build) ──────────────────────
add_custom_command(TARGET theword POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:theword>/assets
    COMMENT "Copying assets to build directory"
)

# ── Linking ───────────────────────────────────────────────────────────────
if(EMSCRIPTEN)
    target_link_options(theword PRIVATE
        "-sUSE_GLFW=3"
        "-sASYNCIFY=1"
        "-sTOTAL_MEMORY=64MB"
        "-sMAX_WEBGL_VERSION=2"
        "-sFORCE_FILESYSTEM=1"
        "-sFETCH=1"
        "-lidbfs.js"
        "--preload-file" "assets"
        "--shell-file" "${CMAKE_SOURCE_DIR}/wasm_shell.html"
    )
    set_target_properties(theword PROPERTIES SUFFIX ".html")
    target_link_libraries(theword PRIVATE raylib)
elseif(ANDROID)
    target_compile_definitions(theword PRIVATE CURL_STATICLIB)
    target_include_directories(theword PRIVATE
        ${curl_SOURCE_DIR}/include
        ${ANDROID_NDK}/sources/android/native_app_glue
    )
    target_compile_definitions(theword_objects PRIVATE CURL_STATICLIB)
    target_include_directories(theword_objects PRIVATE
        ${curl_SOURCE_DIR}/include
        ${ANDROID_NDK}/sources/android/native_app_glue
    )
    target_link_libraries(theword PRIVATE
        raylib
        libcurl_static
        mbedtls
        mbedcrypto
        mbedx509
        android
        EGL
        GLESv2
        log
    )
    target_link_options(theword PRIVATE
        "-Wl,-u,ANativeActivity_onCreate"
        "-Wl,-undefined,dynamic_lookup"
    )
elseif(APPLE)
    target_link_libraries(theword PRIVATE
        raylib
        "-framework Cocoa"
        "-framework IOKit"
    )
    if(CURL_FOUND)
        target_link_libraries(theword PRIVATE CURL::libcurl)
    endif()
elseif(WIN32)
    target_link_libraries(theword PRIVATE
        raylib
        opengl32
        gdi32
        winmm
    )
    if(MINGW AND CURL_FOUND)
        target_compile_definitions(theword PRIVATE CURL_STATICLIB)
        target_compile_definitions(theword_objects PRIVATE CURL_STATICLIB)
        target_include_directories(theword PRIVATE ${curl_SOURCE_DIR}/include)
        target_include_directories(theword_objects PRIVATE ${curl_SOURCE_DIR}/include)
        target_link_libraries(theword PRIVATE libcurl_static)
    elseif(CURL_FOUND)
        target_link_libraries(theword PRIVATE CURL::libcurl)
    endif()
else()
    target_link_libraries(theword PRIVATE
        raylib
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
        m
        pthread
        dl
        rt
        X11
        Xcursor
        Xrandr
        Xi
    )
    if(CURL_FOUND)
        target_link_libraries(theword PRIVATE CURL::libcurl)
        target_link_libraries(theword_test PRIVATE CURL::libcurl)
    endif()
endif()

# ── Test Monitor platform libs ─────────────────────────────────────────────
if(BUILD_TEST_MONITOR AND TARGET test_monitor)
    if(WIN32)
        target_link_libraries(test_monitor PRIVATE raylib opengl32 gdi32 winmm)
    elseif(APPLE)
        target_link_libraries(test_monitor PRIVATE raylib)
    else()
        target_link_libraries(test_monitor PRIVATE
            raylib m pthread dl rt X11 Xcursor Xrandr Xi)
    endif()
endif()
