#!/bin/bash
# ============================================================================
# TheWord Integration Test Runner
# ============================================================================
# Example runner that sources test_helpers.sh and exercises navigation.
# AI agents can use this as a template for writing their own tests.
#
# Usage:
#   ./scripts/test_integration.sh [build_dir]
#
# The build_dir defaults to build_asan.
# Set TEST_MONITOR_BINARY to enable the live test monitor window.
# ============================================================================
set -euo pipefail

cd "$(dirname "$0")/.."
source scripts/test_helpers.sh

BUILD_DIR="${1:-build_asan}"
TEST_MONITOR_BINARY="${TEST_MONITOR_BINARY:-}"

test_init

# Ensure the build exists
if [ ! -f "$BUILD_DIR/theword" ]; then
    echo "Building $BUILD_DIR ..."
    cmake -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Debug \
        -DCMAKE_CXX_FLAGS="-fsanitize=address -fno-omit-frame-pointer" \
        -G "Unix Makefiles" 2>/dev/null
    cmake --build "$BUILD_DIR" --parallel 2>/dev/null
fi

# Optional: start the live monitor
if [ -n "$TEST_MONITOR_BINARY" ]; then
    monitor_start
    monitor_announce RUNNING "Starting integration tests"
fi

# ── Test: Basic navigation ──────────────────────────────────────────────────
echo "=== Test: Navigate from GEN.1 to EXO.1 via center menu ==="

monitor_announce RUNNING "Starting TheWord"
app_start "$BUILD_DIR/theword"
sleep 3

app_wait_for_chapter "GEN.1" 10 && TEST_PASS=$((TEST_PASS+1)) || TEST_FAIL=$((TEST_FAIL+1))

monitor_announce RUNNING "Opening center menu (G)"
app_press g
sleep 1.5

monitor_announce RUNNING "Selecting Books (Enter)"
app_press Return
sleep 1.5

monitor_announce RUNNING "Selecting Exodus (Down + Enter)"
app_press Down
sleep 0.5
app_press Return
sleep 2

monitor_announce RUNNING "Selecting EXO.1 (Enter)"
app_press Return
sleep 2

app_wait_for_chapter "EXO.1" 10 && TEST_PASS=$((TEST_PASS+1)) || TEST_FAIL=$((TEST_FAIL+1))
app_assert_no_crash && TEST_PASS=$((TEST_PASS+1)) || TEST_FAIL=$((TEST_FAIL+1))

# Cleanup
app_stop

if [ -n "$TEST_MONITOR_BINARY" ]; then
    monitor_announce PASS "All integration tests completed"
    sleep 1
fi

test_report
test_cleanup
