cmake_minimum_required(VERSION 3.16)
project(theword C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/libs)

include(FetchContent)

FetchContent_Declare(
    raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/5.0.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

set(BUILD_GAMES OFF)
set(BUILD_EXAMPLES OFF)

FetchContent_MakeAvailable(raylib)

file(GLOB_RECURSE THEWORD_SOURCES
    src/core/*.cpp
    src/data/*.cpp
    src/document/*.cpp
    src/highlight/*.cpp
    src/input/*.cpp
    src/persistence/*.cpp
    src/renderer/*.cpp
    src/text/*.cpp
)

add_executable(theword
    src/main.cpp
    ${THEWORD_SOURCES}
)

file(COPY shaders DESTINATION ${CMAKE_BINARY_DIR})

if(WIN32)
    target_link_libraries(theword PRIVATE
        raylib
        opengl32
        gdi32
        winmm
    )
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
endif()