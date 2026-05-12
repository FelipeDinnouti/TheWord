cmake_minimum_required(VERSION 3.16)
project(OrbitalSimulation)

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

add_executable(game
    src/main.cpp
)

file(COPY shaders DESTINATION ${CMAKE_BINARY_DIR})

target_link_libraries(game PRIVATE
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
