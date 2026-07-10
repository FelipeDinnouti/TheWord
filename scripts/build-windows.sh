#!/bin/bash
# Build TheWord for Windows (cross-compile from Linux via MinGW)
#
# Usage: ./scripts/build-windows.sh [--clean]
#
# Requirements:
#   MinGW cross-compiler: apt install g++-x86-64-linux-gnu binutils-x86-64-linux-gnu
#
#   --clean    Delete build directory before reconfiguring
set -e

BUILD_DIR="build-windows"
CLEAN=false

while [ $# -gt 0 ]; do
    case "$1" in
        --clean)   CLEAN=true; shift ;;
        --help|-h) head -10 "$0"; exit 0 ;;
        *)         echo "Unknown option: $1"; exit 1 ;;
    esac
done

echo "=== Configuring for Windows (MinGW cross-compile) ==="
if $CLEAN; then
    echo "Removing ${BUILD_DIR}/"
    rm -rf "$BUILD_DIR"
fi

cmake --preset windows-mingw > /dev/null && echo "Config OK" || { echo "Config FAILED"; exit 1; }

echo "=== Building ==="
cmake --build --preset windows-mingw

echo "=== Build complete: ${BUILD_DIR}/theword.exe ==="
