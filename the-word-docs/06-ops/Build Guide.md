# Build Guide

> Status: Updated for all platforms | Last Updated: 2026-06-26

## Build Artifacts Overview

| Platform | Build Directory | Output Artifacts |
|----------|----------------|------------------|
| Linux Desktop | `build/` | `theword` (executable), `theword_test` (test executable) |
| Windows (MSYS2) | `build/` | `theword.exe`, `theword_test.exe` |
| Android | `build-android/` | `libtheword.so` → `theword.apk` (signed, in project root) |
| WebAssembly | `build-wasm/` | `theword.html`, `theword.js`, `theword.wasm`, `theword.data` |

Assets (`assets/` and `shaders/`) are copied into the build directory automatically during CMake configure.

---

## Prerequisites

### Linux
```bash
sudo apt install build-essential cmake git libcurl4-openssl-dev
sudo apt install libgl1-mesa-dev libx11-dev libxcursor-dev libxi-dev pkg-config
```

### Windows (MSYS2)
```bash
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-make mingw-w64-x86_64-raylib
```

### Android NDK
```bash
# Download NDK 25.2.9519653 (or later) via Android Studio SDK Manager,
# or manually from developer.android.com/ndk
export ANDROID_NDK=$HOME/Android/Sdk/ndk/25.2.9519653
export ANDROID_SDK=$HOME/Android/Sdk
```
See `the-word-docs/06-ops/Environment Setup.md` for full NDK setup.

### WebAssembly (Emscripten)
```bash
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh
```

---

## Build & Run

### Linux Desktop (default)
```bash
# Configure
cmake -B build -DCMAKE_BUILD_TYPE=Release -G "Unix Makefiles"

# Build
cmake --build build --parallel

# Run
./build/theword

# Run tests
./build/theword_test
```

### Linux Desktop (debug)
```bash
cmake -B build-debug -DCMAKE_BUILD_TYPE=Debug -G "Unix Makefiles"
cmake --build build-debug --parallel
./build-debug/theword
```

### Windows
```bash
# In MSYS2 MINGW64 terminal:
cmake -B build -G "MinGW Makefiles"
cmake --build build
./build/theword.exe
```

### Android (x86_64 emulator)
```bash
# One-step build + package + sign
./scripts/build-android.sh

# Or step-by-step:
cmake -B build-android \
  -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=x86_64 \
  -DANDROID_PLATFORM=android-24 \
  -DCMAKE_BUILD_TYPE=Release \
  -G "Ninja"
cmake --build build-android --parallel
# → produces build-android/libtheword.so
# The build-android.sh script handles APK packaging + signing
```

### Android (arm64-v8a device)
```bash
cmake -B build-android-arm64 \
  -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-24 \
  -DCMAKE_BUILD_TYPE=Release \
  -G "Ninja"
cmake --build build-android-arm64 --parallel
# Package into APK (see scripts/build-android.sh for reference)
```

### WebAssembly
```bash
# Requires Emscripten SDK (emsdk_env.sh sourced)
cmake -B build-wasm \
  -DCMAKE_TOOLCHAIN_FILE=$EMSDK/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -G "Ninja"
cmake --build build-wasm --parallel
# → produces build-wasm/theword.html, .js, .wasm, .data
# Serve with: python3 -m http.server 8080 (from build-wasm/)
```

---

## API Key Setup

1. Sign up at [platform.youversion.com](https://platform.youversion.com)
2. Create an app → copy App Key
3. Create `.env` at project root:
```env
YVP_APP_KEY=your_app_key_here
```

**Without an API key**, the app uses offline USFM Bible files (Bíblia Livre) — full navigation, no fallback text.

**On Android**, `.env` is bundled into the APK by `scripts/build-android.sh`.

---

## Testing

Tests use the [doctest](https://github.com/doctest/doctest) framework (header-only, fetched by CMake).

```bash
# Build and run (Linux)
cmake --build build --parallel && ./build/theword_test

# Run specific test suite
./build/theword_test --test-case="*USFM*"

# List available tests
./build/theword_test --list-tests
```

Current test count: **64 tests** (verified after Phase 1).

---

## Adding New Source Files

CMake uses explicit file lists (no `GLOB_RECURSE`). After adding a new `.cpp`:

```bash
# 1. Add the file to the corresponding source list in CMakeLists.txt
# 2. Delete build cache and reconfigure:
rm -rf build && cmake -B build -DCMAKE_BUILD_TYPE=Release -G "Unix Makefiles"
```

---

## Common Build Issues

| Issue | Solution |
|-------|----------|
| `CMAKE_CXX_COMPILE_OBJECT not set` | Add `CXX` to project languages: `project(theword C CXX)` |
| New .cpp not compiled | `rm -rf build && cmake -B build ...` |
| libcurl not found | Install `libcurl4-openssl-dev` (Linux) or ensure MSYS2 package |
| API returns "Access denied" | Use Bible ID 3034 (BSB), not 111 (NIV) |
| `libraylib.so: cannot open shared` | Run `sudo ldconfig` or set `LD_LIBRARY_PATH` |
| Android NDK not found | Set `ANDROID_NDK` env var; install NDK 25.2+ |
| Emscripten toolchain not found | Source `emsdk_env.sh` before configuring |
