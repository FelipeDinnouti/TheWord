#!/bin/bash
# Test: Navigate to Exodus 14, highlight a word
set -euo pipefail
cd "$(dirname "$0")/.."
source scripts/test_helpers.sh

BUILD_DIR="${1:-build_asan}"
TEST_MONITOR_BINARY="${TEST_MONITOR_BINARY:-}"
test_init

if [ -n "$TEST_MONITOR_BINARY" ]; then
    monitor_start
    monitor_announce RUNNING "Starting test: EXO.14 navigation + highlight"
fi

echo "=== Test: Navigate to EXO.14 and highlight a word ==="

monitor_announce RUNNING "Starting TheWord"
app_start "$BUILD_DIR/theword"
sleep 3
app_wait_for_chapter "GEN.1" && TEST_PASS=$((TEST_PASS+1)) || TEST_FAIL=$((TEST_FAIL+1))

# Navigate: G → Enter (Books) → Down (Exodus) → Enter
monitor_announce RUNNING "Opening center menu (G)"
app_press g; sleep 1.5

monitor_announce RUNNING "Selecting Books (Enter)"
app_press Return; sleep 1.5

monitor_announce RUNNING "Selecting Exodus (Down)"
app_press Down; sleep 0.5

monitor_announce RUNNING "Opening chapter grid (Enter)"
app_press Return; sleep 2

# Grid: chapter 1 is selected by default.
# Navigate to chapter 14: Down(→6) Down(→11) Right(→12) Right(→13) Right(→14)
monitor_announce RUNNING "Navigating grid to EXO.14: Down Down Right Right Right"
app_press Down; sleep 0.3
app_press Down; sleep 0.3
app_press Right; sleep 0.3
app_press Right; sleep 0.3
app_press Right; sleep 0.3

monitor_announce RUNNING "Selecting chapter 14 (Enter)"
app_press Return; sleep 3

app_wait_for_chapter "EXO.14" && TEST_PASS=$((TEST_PASS+1)) || TEST_FAIL=$((TEST_FAIL+1))

# Highlight a word by clicking in the text area
monitor_announce RUNNING "Clicking at (225, 200) to highlight a word"
app_click 225 200
sleep 1

if app_wait_for_log "Highlight saved:" 5; then
    echo "PASS: Highlight created"
    TEST_PASS=$((TEST_PASS+1))
else
    echo "FAIL: No highlight created"
    TEST_FAIL=$((TEST_FAIL+1))
fi

app_assert_no_crash && TEST_PASS=$((TEST_PASS+1)) || TEST_FAIL=$((TEST_FAIL+1))

app_stop
if [ -n "$TEST_MONITOR_BINARY" ]; then
    monitor_announce PASS "All tests completed"
    sleep 1
fi

test_report
test_cleanup
