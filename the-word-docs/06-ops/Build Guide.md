# Build Guide

> Status: Stable | Last Updated: 2026-06-21

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

## Build & Run

```bash
# Configure
cmake -B build -DCMAKE_BUILD_TYPE=Release -G "Unix Makefiles"

# Build
cmake --build build --parallel

# Run
./build/theword
```

**Windows:**
```bash
cmake -B build -G "MinGW Makefiles"
cmake --build build
./build/theword.exe
```

## API Key Setup

1. Sign up at [platform.youversion.com](https://platform.youversion.com)
2. Create an app → copy App Key
3. Create `.env` at project root:
```env
YVP_APP_KEY=your_app_key_here
```

Without an API key, the app shows John 3:16-18 fallback text.

## Adding New Source Files

Delete the `build/` folder and reconfigure:
```bash
rm -rf build && cmake -B build -DCMAKE_BUILD_TYPE=Release -G "Unix Makefiles"
```

## Testing

```bash
cmake --build build --parallel && ./build/theword
```

## Android Build (Phase 8)

Android support is planned but not yet implemented. It will require:

1. Installing the Android NDK
2. Creating a CMake toolchain file targeting `arm64-v8a` / `armeabi-v7a`
3. A separate build directory or CMake preset for Android
4. Raylib must be cross-compiled for Android (FetchContent works, but the toolchain must be set before `project()`)

**Expected commands (not yet functional):**
```bash
cmake -B build-android \
  -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-24 \
  -G "Ninja"
cmake --build build-android
```

**Recommended intermediate step:** Build a WebAssembly target via Emscripten before tackling Android. See `the-word-docs/03-modules/UI Layer.md` for details.
