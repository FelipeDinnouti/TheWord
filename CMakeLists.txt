cmake_minimum_required(VERSION 3.16)
project(raylib_project)

set(CMAKE_CXX_STANDARD 17)

# Find Raylib 
# find_package(raylib REQUIRED) # WINDOWS

find_package(PkgConfig REQUIRED)
#  pkg_check_modules(RAYLIB REQUIRED raylib) # LINUX

# additional dependencies
include_directories(/libs)

# Create the executable
add_executable(game src/main.cpp)

# Copy shaders to build directory
file(COPY shaders DESTINATION ${CMAKE_BINARY_DIR})


## WINDOWS

# Link Raylib and Windows libraries
target_link_libraries(game PRIVATE raylib opengl32 gdi32 winmm)

## LINUX

# Link Raylib and Linux dependencies
# target_link_libraries(game PRIVATE 
#     raylib 
#     GL       # OpenGL
#     m        # Math library
#     pthread  # Threading
#     dl       # Dynamic linking
#     rt       # Real-time extensions
# )
