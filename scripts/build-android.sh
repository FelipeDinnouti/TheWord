#!/bin/bash
# Build TheWord for Android: cross-compile C++ → .so → signed APK
#
# Usage: ./scripts/build-android.sh [x86_64|arm64-v8a]
#   Default: x86_64 (emulator). arm64-v8a for physical devices.
#   armeabi-v7a dropped because raylib 5.0 NEON half-float intrinsics require ARMv8+.
#   Default: x86_64 (emulator)
#
# Pipeline overview:
#   1. CMake via NDK toolchain — cross-compiles C++ to target ABI
#   2. ninja build — produces libtheword.so
#   3. javac + d8 — compiles TheWordActivity.java → classes.dex (optional)
#   4. aapt — creates base APK from AndroidManifest.xml
#   5. zip — inserts .so, .dex, assets, shaders into APK
#   6. zipalign — 4-byte aligns for mmap
#   7. apksigner — signs with debug keystore
#
# Requirements:
#   ANDROID_NDK — path to NDK (e.g. ~/Android/Sdk/ndk/25.2.9519653)
#   ANDROID_SDK — path to SDK (e.g. ~/Android/Sdk)
#   javac (JDK 11+) — optional, for Java activity compilation
set -e

ABI="${1:-x86_64}"

case "$ABI" in
    x86_64)        LIB_DIR="x86_64"    ;;
    arm64-v8a)     LIB_DIR="arm64-v8a"  ;;
    *)
        echo "Usage: $0 [x86_64|arm64-v8a]"
        exit 1
        ;;
esac

# Read version from CMakeLists.txt (project() line, not cmake_minimum_required)
VERSION=$(grep -oP 'project\(\w+ VERSION \K[0-9.]+' CMakeLists.txt)
[ -z "$VERSION" ] && VERSION="0.0.0"

ANDROID_NDK="${ANDROID_NDK:-$HOME/Android/Sdk/ndk/25.2.9519653}"
ANDROID_SDK="${ANDROID_SDK:-$HOME/Android/Sdk}"
CMAKE_TOOLCHAIN="$ANDROID_NDK/build/cmake/android.toolchain.cmake"
BUILD_DIR="build-android-${ABI}"
APK_DIR="/tmp/apk-$$"
NDK_SYSROOT="$ANDROID_NDK/toolchains/llvm/prebuilt/linux-x86_64/sysroot"

if [ ! -f "$CMAKE_TOOLCHAIN" ]; then
    echo "Error: Android NDK toolchain not found at $CMAKE_TOOLCHAIN"
    echo "Set ANDROID_NDK environment variable or install NDK 25.2.9519653"
    exit 1
fi

# ── Step 1-2: CMake configure + ninja build ──────────────────────────────────
# android.toolchain.cmake swaps the system compiler for aarch64-linux-android-*
# cross-compiler. -DPLATFORM=Android tells raylib to use EGL/GLESv2.
# -DANDROID_PLATFORM=android-24 sets min API level (Android 7.0).
# libcurl is built from source via FetchContent (no system libcurl on Android).
echo "=== Configuring for ${ABI} with NDK: $ANDROID_NDK ==="
cmake -B "$BUILD_DIR" \
    -DCMAKE_TOOLCHAIN_FILE="$CMAKE_TOOLCHAIN" \
    -DANDROID_ABI="${ABI}" \
    -DANDROID_PLATFORM=android-24 \
    -DCMAKE_BUILD_TYPE=Release \
    -DPLATFORM=Android \
    -DANDROID_NDK="$ANDROID_NDK" \
    -DOPENGL_INCLUDE_DIR="$NDK_SYSROOT/usr/include" \
    -G "Ninja"

echo "=== Building ==="
cmake --build "$BUILD_DIR" --parallel

# ── Step 3: Java compilation (optional) ──────────────────────────────────────
# TheWordActivity.java extends android.app.NativeActivity and calls
# System.loadLibrary("theword"). Without it, Android falls back to the default
# NativeActivity which also works but lacks custom splash/IME handling.
#
# javac compiles .java → .class.
# d8 converts .class → classes.dex (Dalvik Executable, what ART actually runs).
#
# BUILD_TOOLS and PLATFORM_JAR are defined here (before step 4) so javac can use
# $PLATFORM_JAR and $BUILD_TOOLS/* are available for d8.
echo "=== Compiling Java Activity ==="
BUILD_TOOLS="$ANDROID_SDK/build-tools/34.0.0"
PLATFORM_JAR="$ANDROID_SDK/platforms/android-24/android.jar"
JAVA_SRC="src/main/java/com/theword/app/TheWordActivity.java"
JAVA_OBJ="/tmp/java-obj-$$"
mkdir -p "$JAVA_OBJ"
if [ -f "$JAVA_SRC" ] && command -v javac &>/dev/null; then
    javac -cp "$PLATFORM_JAR" -d "$JAVA_OBJ" "$JAVA_SRC"
    D8="$BUILD_TOOLS/d8"
    DX="$BUILD_TOOLS/dx"
    if [ -x "$D8" ]; then
        "$D8" --lib "$PLATFORM_JAR" --output "$JAVA_OBJ" "$JAVA_OBJ/com/theword/app/TheWordActivity.class"
    elif [ -x "$DX" ]; then
        "$DX" --dex --output="$JAVA_OBJ/classes.dex" "$JAVA_OBJ"
    else
        echo "Warning: Neither d8 nor dx found. Skipping Java compilation."
        rm -rf "$JAVA_OBJ"
        JAVA_OBJ=""
    fi
else
    echo "Warning: Java source or javac not found. Using NativeActivity only."
    rm -rf "$JAVA_OBJ"
    JAVA_OBJ=""
fi

# ── Step 4-5: APK packaging ──────────────────────────────────────────────────
# aapt creates a skeleton APK from AndroidManifest.xml.
# Then we manually insert native libs, classes.dex, and assets into a ZIP.
# Required structure: lib/<abi>/libtheword.so, classes.dex (root), assets/.
echo "=== Packaging APK ==="
$BUILD_TOOLS/aapt package -f -M AndroidManifest.xml \
    -I "$PLATFORM_JAR" \
    -F /tmp/theword-base-$$.apk

cp /tmp/theword-base-$$.apk /tmp/theword-unsigned-$$.apk

mkdir -p "$APK_DIR/lib/${LIB_DIR}"
cp "$BUILD_DIR/libtheword.so" "$APK_DIR/lib/${LIB_DIR}/"
if [ -n "$JAVA_OBJ" ] && [ -f "$JAVA_OBJ/classes.dex" ]; then
    cp "$JAVA_OBJ/classes.dex" "$APK_DIR/"
fi
cp -rL assets "$APK_DIR/"
cp -rL shaders "$APK_DIR/" 2>/dev/null || true
[ -f .env ] && cp .env "$APK_DIR/assets/"
(cd "$APK_DIR" && zip -r /tmp/theword-unsigned-$$.apk lib/ classes.dex assets/ shaders/ 2>/dev/null)

# ── Step 6: zipalign ─────────────────────────────────────────────────────────
# 4-byte alignment allows Android to mmap() ZIP entries directly into memory
# without copying. Required by Google Play and Android 11+.
$BUILD_TOOLS/zipalign -f -v 4 /tmp/theword-unsigned-$$.apk /tmp/theword-aligned-$$.apk

mkdir -p dist

# ── Step 7: sign ─────────────────────────────────────────────────────────────
# Every APK must be signed. Debug keystore is self-signed (created automatically
# if missing). For Play Store distribution, replace with a release keystore.
KEYSTORE="${KEYSTORE:-$HOME/.android/debug.keystore}"
if [ ! -f "$KEYSTORE" ]; then
    mkdir -p "$HOME/.android"
    keytool -genkey -v -keystore "$KEYSTORE" -alias androiddebugkey \
        -keyalg RSA -keysize 2048 -validity 10000 \
        -storepass android -keypass android \
        -dname "CN=, OU=, O=, L=, S=, C="
fi

$BUILD_TOOLS/apksigner sign --ks "$KEYSTORE" \
    --ks-pass pass:android --key-pass pass:android \
    --min-sdk-version 24 \
    --out "dist/theword-${ABI}-v${VERSION}.apk" /tmp/theword-aligned-$$.apk

rm -rf "$APK_DIR" /tmp/theword-*-$$.apk

echo "=== APK ready: dist/theword-${ABI}-v${VERSION}.apk ($(stat -c%s "dist/theword-${ABI}-v${VERSION}.apk") bytes) ==="
