# Environment Setup

> Status: Updated for all platforms | Last Updated: 2026-06-26

## Linux Desktop

```bash
# Build essentials
sudo apt update
sudo apt install build-essential cmake git

# libcurl (needed for online Bible API)
sudo apt install libcurl4-openssl-dev

# Graphics dependencies (Raylib)
sudo apt install libgl1-mesa-dev libx11-dev libxcursor-dev libxi-dev pkg-config
```

Raylib is downloaded automatically via FetchContent during CMake configure.

## Windows (MSYS2)

1. Install MSYS2 from [msys2.org](https://www.msys2.org/)
2. Open **MSYS2 MINGW64** terminal (critical — not MSYS terminal)
3. Run:
```bash
pacman -Syu   # Update core packages
pacman -Su    # Complete update
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-make mingw-w64-x86_64-raylib
```

## Android NDK

The Android build requires the NDK (Native Development Kit) and SDK build tools.

### Option A: Android Studio (recommended)
1. Install [Android Studio](https://developer.android.com/studio)
2. Open **SDK Manager** → **SDK Tools** → check **NDK (Side by side)**
3. Select NDK 25.2.9519653 (or latest 25.x)
4. Note the install path (usually `~/Android/Sdk/ndk/25.2.9519653`)

### Option B: Command-line only
```bash
# Download command-line tools
wget https://dl.google.com/android/repository/commandlinetools-linux-11076708_latest.zip
unzip commandlinetools-linux-*.zip -d ~/Android/Sdk/cmdline-tools
mv ~/Android/Sdk/cmdline-tools/cmdline-tools ~/Android/Sdk/cmdline-tools/latest

# Install NDK
~/Android/Sdk/cmdline-tools/latest/bin/sdkmanager --install "ndk;25.2.9519653"

# Install build tools (for APK packaging)
~/Android/Sdk/cmdline-tools/latest/bin/sdkmanager --install "build-tools;34.0.0"
~/Android/Sdk/cmdline-tools/latest/bin/sdkmanager --install "platforms;android-24"
```

### Environment variables
```bash
export ANDROID_NDK=$HOME/Android/Sdk/ndk/25.2.9519653
export ANDROID_SDK=$HOME/Android/Sdk
```

## WebAssembly (Emscripten)

```bash
# Clone the SDK
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk

# Install and activate latest
./emsdk install latest
./emsdk activate latest

# Source the environment (add to ~/.bashrc for persistence)
source ./emsdk_env.sh
```

The WASM build requires Ninja:
```bash
sudo apt install ninja-build   # Linux
# or: pacman -S ninja            # MSYS2
```

## Verifying Setup

```bash
# Check libcurl
curl --version

# Check NDK (Android)
ls $ANDROID_NDK/build/cmake/android.toolchain.cmake

# Check Emscripten (WASM)
which emcmake
emcc --version

# Test Linux build
cmake -B build -DCMAKE_BUILD_TYPE=Release -G "Unix Makefiles"
cmake --build build --parallel
./build/theword_test   # All 64 tests should pass
```
