# Comprehensive Guide: Raylib with CMake on Linux vs. Windows

## I. Key Differences Between Linux and Windows

### 1. Installation Methods
| **Linux**                                   | **Windows**                                |
|---------------------------------------------|--------------------------------------------|
| `sudo apt install libraylib-dev` (limited)  | Precompiled binaries from GitHub releases  |
| Build from source (recommended)             | MSYS2: `pacman -S mingw-w64-x86_64-raylib` |
| Requires manual CMake config                | Automatic CMake config via MSYS2 paths     |

### 2. Library Dependencies
| **Linux**               | **Windows**           |
|-------------------------|-----------------------|
| `GL` (OpenGL)           | `opengl32`           |
| `m` (math library)      | `gdi32`              |
| `pthread` (threading)   | `winmm` (Windows MM) |
| `dl` (dynamic linking)  |                       |
| `rt` (real-time)        |                       |

### 3. File Paths
| **Component**        | **Linux**                  | **Windows (MSYS2)**             |
|----------------------|----------------------------|----------------------------------|
| Raylib headers       | `/usr/local/include`       | `C:/msys64/mingw64/include`     |
| Raylib libraries     | `/usr/local/lib`           | `C:/msys64/mingw64/lib`         |
| CMake config files   | `/usr/local/lib/cmake/raylib` | Rarely included in MSYS2 install|

### 4. Runtime Behavior
| **Aspect**          | **Linux**                              | **Windows**                      |
|---------------------|----------------------------------------|----------------------------------|
| Executable format   | ELF binary (no extension)              | PE executable (.exe)             |
| Library loading     | `LD_LIBRARY_PATH`/`ldconfig`           | DLLs in same folder as .exe      |
| Default assets path | `./` or `GetWorkingDirectory()`        | `.exe` directory                 |

## II. Linux-Specific Setup Guide

### Step 1: Install Dependencies
```bash
# Essential build tools
sudo apt install build-essential cmake git

# Graphics dependencies
sudo apt install libgl1-mesa-dev libx11-dev libxcursor-dev libxi-dev

# For pkg-config support
sudo apt install pkg-config
```

### Step 2: Install Raylib (Recommended Method)
```bash
git clone https://github.com/raysan5/raylib
cd raylib
mkdir build && cd build

# Configure with CMake support
cmake -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_PREFIX=/usr/local ..

# Build and install
make -j8
sudo make install

# Update library cache
sudo ldconfig
```

### Step 3: Verify Installation
```bash
# Check library presence
ls /usr/local/lib/libraylib.so*

# Verify pkg-config
pkg-config --modversion raylib
```

## III. CMake Configuration Fixes

### Linux-Optimized CMakeLists.txt
```cmake
cmake_minimum_required(VERSION 3.16)
project(raylib_project)

set(CMAKE_CXX_STANDARD 17)

# Linux-specific configuration
if(UNIX AND NOT APPLE)
    # Use pkg-config for reliability
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(RAYLIB REQUIRED raylib)
    
    # Set include directories
    include_directories(${RAYLIB_INCLUDE_DIRS})
    
    # Define link libraries
    set(RAYLIB_LIBS ${RAYLIB_LIBRARIES} GL m pthread dl rt)
endif()

add_executable(game src/main.cpp)

# Platform-specific linking
if(WIN32)
    target_link_libraries(game PRIVATE raylib opengl32 gdi32 winmm)
else()
    target_link_libraries(game PRIVATE ${RAYLIB_LIBS})
endif()

# Asset handling
file(COPY assets DESTINATION ${CMAKE_BINARY_DIR})
file(COPY shaders DESTINATION ${CMAKE_BINARY_DIR})
```

### Windows Compatibility
```cmake
# Windows-specific section
if(WIN32)
    find_package(raylib REQUIRED)
    # MSYS2 path hint (if needed)
    # find_package(raylib REQUIRED PATHS "C:/msys64/mingw64/lib/cmake/raylib")
endif()
```

## IV. Common Linux Errors & Fixes

### 1. "Unknown CMake command pkg_check_modules"
**Fix:**
```cmake
# Add BEFORE pkg_check_modules
find_package(PkgConfig REQUIRED)
```

### 2. "libraylib.so.xxx: cannot open shared object file"
**Fixes:**
```bash
# Temporary fix (terminal session):
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH

# Permanent fix:
sudo ldconfig

# For custom install paths:
echo "/custom/path" | sudo tee /etc/ld.so.conf.d/raylib.conf
sudo ldconfig
```

### 3. Undefined OpenGL references
**Fix:** Ensure all dependencies are installed:
```bash
sudo apt install libgl1-mesa-dev libx11-dev
```

### 4. CMake can't find Raylib
**Solutions:**
```bash
# Verify installation
sudo find / -name '*raylib*' 2>/dev/null

# Specify path in CMake:
cmake -B build -DCMAKE_PREFIX_PATH=/usr/local
```

## V. Build & Execution Commands

### Linux Build Process
```bash
# Configure
cmake -B build

# Build
cmake --build build --parallel

# Run
./build/game
```

### Windows Build Process (MSYS2)
```bash
# Configure
cmake -B build -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH="C:/msys64/mingw64"

# Build
cmake --build build

# Run
./build/game.exe
```

## VI. Debugging Tools

### Linux Diagnostic Commands
```bash
# Check library dependencies
ldd ./build/game

# Search for Raylib files
sudo find / -name '*raylib*' 2>/dev/null

# Check installed packages
apt list --installed | grep raylib

# View linker paths
ldconfig -v | grep raylib
```

### Windows Diagnostic Commands (MSYS2)
```bash
# Verify Raylib install
pacman -Q | grep raylib

# Check DLL dependencies
ntldd ./build/game.exe
```

## VII. Static Linking Alternative

### Build Raylib Statically
```bash
cd raylib/build
cmake -DBUILD_SHARED_LIBS=OFF ..
make
sudo make install
```

### CMake Modification
```cmake
# Add to CMakeLists.txt
if(UNIX)
    set(CMAKE_EXE_LINKER_FLAGS "-static")
endif()
```

## VIII. Key Best Practices

1. **Always build Raylib from source** on Linux for full CMake support
2. **Use `pkg-config`** instead of `find_package` on Linux
3. **Run `sudo ldconfig`** after installing libraries
4. **Include asset copying** in CMake for cross-platform compatibility
5. **Use versioned paths** (`/usr/local`) instead of `/usr` for custom installs
6. **Check library paths** with `ldd`/`ntldd` before distribution
7. **Consider static linking** for simplified deployment

This documentation covers all critical differences, setup steps, and troubleshooting techniques for Raylib development on Linux compared to Windows. The solutions prioritize reliability and cross-platform compatibility while addressing common pain points specific to Linux environments.