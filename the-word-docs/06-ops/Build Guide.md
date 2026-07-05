# Build Guide

> Status: Updated for all platforms | Last Updated: 2026-06-26

## Build Artifacts Overview

| Platform | Build Directory | How to configure | Output Artifacts |
|----------|----------------|------------------|------------------|
| Linux Desktop | `build/` | `cmake --preset default` | `theword` (executable), `theword_test` (test executable) |
| Linux Debug | `build-debug/` | `cmake --preset debug` | `theword`, `theword_test` |
| Android (x86_64) | `build-android-x86_64/` | `cmake --preset android-x86_64` or `./scripts/build-android.sh x86_64` | `libtheword.so` → `theword-x86_64.apk` |
| Android (arm64) | `build-android-arm64/` | `cmake --preset android-arm64` or `./scripts/build-android.sh arm64-v8a` | `libtheword.so` → `theword-arm64-v8a.apk` |
| WebAssembly | `build-wasm/` | `cmake --preset wasm` or `./scripts/build-wasm.sh` | `theword.html`, `theword.js`, `theword.wasm`, `theword.data` |
| Windows (cross) | `build-windows/` | `cmake --preset windows-mingw` | `theword.exe` |

Assets (`assets/` and `shaders/`) are copied into the build directory automatically during build.

---

## Prerequisites

### Linux
```bash
sudo apt install build-essential cmake git libcurl4-openssl-dev
sudo apt install libgl1-mesa-dev libx11-dev libxcursor-dev libxi-dev pkg-config
```

libcurl is optional — the app works in USFM-only mode without it.

### Windows (MSYS2)
```bash
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-make mingw-w64-x86_64-raylib
```

### Android NDK + SDK
```bash
# Required: NDK 25.2.9519653 (or later)
export ANDROID_NDK=$HOME/Android/Sdk/ndk/25.2.9519653
# Required for APK packaging: Android SDK build-tools + platform
export ANDROID_SDK=$HOME/Android/Sdk
# Required for Java compilation step: JDK 11+ (for javac and d8)
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

## Android-Specific: Raylib Patch for Text Input

When building for Android, CMake automatically applies a patch to raylib 5.0's `rcore_android.c` (see `cmake/patches/raylib-5.0-android-char-input.patch`). This patch:

1. Fixes a gamepad/keyboard source flag collision that can swallow keyboard events on some devices (upstream raylib issue [#5387](https://github.com/raysan5/raylib/issues/5387)).
2. Adds JNI-based Unicode character capture so `GetCharPressed()` returns text input instead of always returning 0.

The patch requires the `patch` command-line tool. CMake will error at configure time if it's not found. Install it with:

```bash
sudo apt install patch        # Debian/Ubuntu
sudo pacman -S patch          # Arch
brew install patch            # macOS
```

The patch is NOT upstream in raylib 5.0. It is maintained locally and applied per-build. If you upgrade raylib to a newer version, verify and update the patch accordingly.

## Build & Run

### Quick Reference — CMake Presets

All presets are defined in `CMakePresets.json` at the project root.

```bash
# List available presets
cmake --list-presets

# Configure + build with a preset
cmake --preset <name>
cmake --build --preset <name>
```

### Linux Desktop (default)
```bash
cmake --preset default
cmake --build --preset default
./build/theword
```

### Linux Desktop (debug)
```bash
cmake --preset debug
cmake --build --preset debug
./build-debug/theword
```

### Linux Desktop (legacy — without presets)
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -G "Unix Makefiles"
cmake --build build --parallel
./build/theword
```

### Windows (MSYS2)
```bash
# In MSYS2 MINGW64 terminal:
cmake -B build -G "MinGW Makefiles"
cmake --build build
./build/theword.exe
```

### Android

The Android build requires the NDK and SDK (see Environment Setup). The build script handles everything: CMake configure → native build → Java compilation → APK packaging → signing.

```bash
# One-command build for emulator (x86_64)
./scripts/build-android.sh x86_64

# One-command build for device (arm64-v8a)
./scripts/build-android.sh arm64-v8a

# Install on connected device
adb install theword-x86_64-v1.1.0.apk     # emulator
adb install theword-arm64-v8a-v1.1.0.apk  # device
```

Or step-by-step using a preset:

```bash
# Configure (requires ANDROID_NDK and ANDROID_SDK env vars)
cmake --preset android-arm64

# Build native library
cmake --build --preset android-arm64

# The shared library is at build-android-arm64/libtheword.so
# Package into APK manually:
#   (see scripts/build-android.sh for the full packaging pipeline)
```

### WebAssembly

Requires Emscripten SDK (`emsdk_env.sh` sourced). The build script wraps configure + build:

```bash
# One-command build
./scripts/build-wasm.sh

# Or step-by-step:
cmake --preset wasm
cmake --build --preset wasm

# Serve locally
python3 -m http.server 8080 -d build-wasm
# Then open http://localhost:8080/theword.html
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

Current test count: **64 tests**.

---

## Adding New Source Files

CMake uses explicit file lists (no `GLOB_RECURSE`). After adding a new `.cpp`:

```bash
# 1. Add the file to the corresponding source list in CMakeLists.txt
# 2. Delete build cache and reconfigure:
rm -rf build && cmake --preset default
```

---

## Common Build Issues

| Issue | Solution |
|-------|----------|
| `CMAKE_CXX_COMPILE_OBJECT not set` | Add `CXX` to project languages: `project(theword C CXX)` |
| New .cpp not compiled | `rm -rf build && cmake --preset default` |
| libcurl not found | Install `libcurl4-openssl-dev` (Linux) or ensure MSYS2 package — the build continues without it (USFM-only mode) |
| API returns "Access denied" | Use Bible ID 3034 (BSB), not 111 (NIV) |
| `libraylib.so: cannot open shared` | Run `sudo ldconfig` or set `LD_LIBRARY_PATH` |
| Android NDK not found | Set `ANDROID_NDK` env var; install NDK 25.2+ |
| `javac` not found (Android) | Install JDK 11+: `sudo apt install openjdk-11-jdk` |
| `d8` not found (Android) | Install Android SDK build-tools 34.0.0 via sdkmanager |
| APK packaging fails (Android) | Ensure `ANDROID_SDK` is set and points to valid SDK directory |
| Emscripten toolchain not found | Source `emsdk_env.sh` before configuring |
| `ninja` not found | Install `ninja-build` (Linux) or `ninja` (MSYS2) |
