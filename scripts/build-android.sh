#!/bin/bash
# Build TheWord for Android NDK and package APK
set -e

ANDROID_NDK="${ANDROID_NDK:-$HOME/Android/Sdk/ndk/25.2.9519653}"
ANDROID_SDK="${ANDROID_SDK:-$HOME/Android/Sdk}"
CMAKE_TOOLCHAIN="$ANDROID_NDK/build/cmake/android.toolchain.cmake"
BUILD_DIR="build-android"
APK_DIR="/tmp/apk-$$"
NDK_SYSROOT="$ANDROID_NDK/toolchains/llvm/prebuilt/linux-x86_64/sysroot"

if [ ! -f "$CMAKE_TOOLCHAIN" ]; then
    echo "Error: Android NDK toolchain not found at $CMAKE_TOOLCHAIN"
    echo "Set ANDROID_NDK environment variable or install NDK 25.2.9519653"
    exit 1
fi

echo "=== Configuring with NDK: $ANDROID_NDK ==="
cmake -B "$BUILD_DIR" \
    -DCMAKE_TOOLCHAIN_FILE="$CMAKE_TOOLCHAIN" \
    -DANDROID_ABI=arm64-v8a \
    -DANDROID_PLATFORM=android-24 \
    -DCMAKE_BUILD_TYPE=Release \
    -DPLATFORM=Android \
    -DOPENGL_INCLUDE_DIR="$NDK_SYSROOT/usr/include" \
    -G "Ninja"

echo "=== Building ==="
cmake --build "$BUILD_DIR" --parallel

echo "=== Packaging APK ==="
BUILD_TOOLS="$ANDROID_SDK/build-tools/34.0.0"
PLATFORM_JAR="$ANDROID_SDK/platforms/android-24/android.jar"

$BUILD_TOOLS/aapt package -f -M AndroidManifest.xml \
    -I "$PLATFORM_JAR" \
    -F /tmp/theword-base-$$.apk

cp /tmp/theword-base-$$.apk /tmp/theword-unsigned-$$.apk

$BUILD_TOOLS/aapt add /tmp/theword-unsigned-$$.apk \
    "$BUILD_DIR/libtheword.so"

# Build full APK dir and zip assets
mkdir -p "$APK_DIR"
cp -rL assets "$APK_DIR/"
cp -rL shaders "$APK_DIR/" 2>/dev/null || true
(cd "$APK_DIR" && zip -r /tmp/theword-unsigned-$$.apk assets/ shaders/ 2>/dev/null)

$BUILD_TOOLS/zipalign -f -v 4 /tmp/theword-unsigned-$$.apk /tmp/theword-aligned-$$.apk

# Debug keystore (create if missing)
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
    --out "theword.apk" /tmp/theword-aligned-$$.apk

rm -rf "$APK_DIR" /tmp/theword-*-$$.apk

echo "=== APK ready: theword.apk ($(stat -c%s theword.apk) bytes) ==="
