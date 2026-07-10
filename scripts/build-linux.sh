#!/bin/bash
# Build TheWord for Linux Desktop (Release or Debug)
#
# Usage: ./scripts/build-linux.sh [--debug] [--clean] [--test]
#
#   --debug    Build Debug configuration (default: Release)
#   --clean    Delete build directory before reconfiguring
#   --test     Run tests after build (Release only)
#
# Examples:
#   ./scripts/build-linux.sh                       # Release build
#   ./scripts/build-linux.sh --debug               # Debug build
#   ./scripts/build-linux.sh --clean               # Full reconfigure
#   ./scripts/build-linux.sh --test                # Build + run tests
#   ./scripts/build-linux.sh --debug --clean       # Debug + reconfigure
set -e

BUILD_TYPE="Release"
BUILD_DIR="build"
CLEAN=false
RUN_TESTS=false

while [ $# -gt 0 ]; do
    case "$1" in
        --debug)   BUILD_TYPE="Debug"; BUILD_DIR="build-debug"; shift ;;
        --clean)   CLEAN=true; shift ;;
        --test)    RUN_TESTS=true; shift ;;
        --help|-h) head -20 "$0"; exit 0 ;;
        *)         echo "Unknown option: $1"; exit 1 ;;
    esac
done

PRESET="default"
[ "$BUILD_TYPE" = "Debug" ] && PRESET="debug"

echo "=== Configuring for ${BUILD_TYPE} ==="
if $CLEAN; then
    echo "Removing ${BUILD_DIR}/"
    rm -rf "$BUILD_DIR"
fi

cmake --preset "$PRESET" > /dev/null && echo "Config OK" || { echo "Config FAILED"; exit 1; }

echo "=== Building ==="
cmake --build --preset "$PRESET"

echo "=== Build complete: ${BUILD_DIR}/theword ==="

if $RUN_TESTS; then
    echo "=== Running tests ==="
    "./${BUILD_DIR}/theword_test" --test-case-exclude="*Locale*"
    echo "=== Tests OK ==="
fi
