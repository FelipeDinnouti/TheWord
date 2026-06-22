# Environment Setup

> Status: Stable | Last Updated: 2026-06-21

## Linux Setup

```bash
# Install build essentials
sudo apt update
sudo apt install build-essential cmake git

# Install libcurl
sudo apt install libcurl4-openssl-dev

# Install graphics dependencies (for Raylib)
sudo apt install libgl1-mesa-dev libx11-dev libxcursor-dev libxi-dev pkg-config
```

Raylib is downloaded automatically via FetchContent during CMake configure.

## Windows Setup (MSYS2)

1. Install MSYS2 from [msys2.org](https://www.msys2.org/)
2. Open **MSYS2 MINGW64** terminal (critical — not MSYS terminal)
3. Run:
```bash
pacman -Syu   # Update core packages
pacman -Su    # Complete update
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-make mingw-w64-x86_64-raylib
```

## Verifying Setup

```bash
# Check libcurl
curl --version

# Test build
cmake -B build -DCMAKE_BUILD_TYPE=Release -G "Unix Makefiles"
cmake --build build --parallel
```
