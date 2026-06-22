cmake_minimum_required(VERSION 3.16)
project(theword C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# ── Raylib ─────────────────────────────────────────────────────────────────
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
set(CORE_SOURCES
    src/core/APIClient.cpp
    src/core/EnvLoader.cpp
    src/core/FontHelper.cpp
)

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

target_include_directories(theword PRIVATE src ${sqlite3_SOURCE_DIR})
target_include_directories(theword_test PRIVATE src ${doctest_SOURCE_DIR} ${sqlite3_SOURCE_DIR})

# ── Asset copying ─────────────────────────────────────────────────────────
file(COPY assets DESTINATION ${CMAKE_BINARY_DIR})
file(COPY shaders DESTINATION ${CMAKE_BINARY_DIR})

# ── Linking ───────────────────────────────────────────────────────────────
if(WIN32)
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
