#!/bin/bash
# Build TheWord for WebAssembly via Emscripten SDK
#
# Pipeline overview:
#   1. CMake via Emscripten toolchain — cross-compiles C++ to WebAssembly
#   2. ninja build — produces theword.html, .js, .wasm, .data (preloaded assets)
#
# Requirements:
#   EMSDK — path to Emscripten SDK (default: $HOME/emsdk)
#   ninja — build system (sudo apt install ninja-build)
#
# Before running:
#   cd emsdk && ./emsdk install latest && ./emsdk activate latest
#   source ./emsdk_env.sh
set -e

EMSDK="${EMSDK:-$HOME/emsdk}"
BUILD_DIR="build-wasm"

if [ ! -f "$EMSDK/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake" ]; then
    echo "Error: Emscripten SDK not found at $EMSDK"
    echo "Set EMSDK environment variable or install from https://emscripten.org"
    exit 1
fi

# ── Step 1-2: CMake configure + ninja build ──────────────────────────────────
# Emscripten.cmake swaps the system compiler for emcc/em++ (LLVM→Wasm backend).
# -sUSE_GLFW=3 and -sASYNCIFY=1 are set in CMakeLists.txt for raylib compat.
# -lidbfs.js enables IndexedDB-backed persistent filesystem (for SQLite).
# --preload-file assets embeds assets/ into the .data file loaded at startup.
echo "=== Configuring WASM build ==="
cmake -B "$BUILD_DIR" \
    -DCMAKE_TOOLCHAIN_FILE="$EMSDK/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake" \
    -DCMAKE_BUILD_TYPE=Release \
    -G "Ninja"

echo "=== Building ==="
cmake --build "$BUILD_DIR" --parallel

echo "=== Output ==="
ls -lh "$BUILD_DIR"/theword.html "$BUILD_DIR"/theword.js "$BUILD_DIR"/theword.wasm "$BUILD_DIR"/theword.data 2>/dev/null
echo ""
echo "Serve with: python3 -m http.server 8080 -d $BUILD_DIR"
