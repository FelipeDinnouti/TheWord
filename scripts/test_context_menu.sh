#!/bin/bash
# Test: Context menu on highlight — right-click → "Del" button → delete
set -euo pipefail
cd "$(dirname "$0")/.."
source scripts/test_helpers.sh

BUILD_DIR="${1:-build_asan}"
TEST_MONITOR_BINARY="${TEST_MONITOR_BINARY:-}"
test_init

if [ -n "$TEST_MONITOR_BINARY" ]; then
    monitor_start
    monitor_announce RUNNING "Starting context menu test"
fi

echo "=== Test: Context menu — highlight, right-click, delete ==="

monitor_announce RUNNING "Starting TheWord"
app_start "$BUILD_DIR/theword"
sleep 3
app_wait_for_chapter "GEN.1" && TEST_PASS=$((TEST_PASS+1)) || TEST_FAIL=$((TEST_FAIL+1))

# Navigate to EXO.14 via center menu (same as before)
monitor_announce RUNNING "Opening center menu (G)"
app_press g; sleep 1.5

monitor_announce RUNNING "Selecting Books (Enter)"
app_press Return; sleep 1.5

monitor_announce RUNNING "Selecting Exodus (Down)"
app_press Down; sleep 0.5

monitor_announce RUNNING "Opening chapter grid (Enter)"
app_press Return; sleep 2

monitor_announce RUNNING "Navigating grid to EXO.14: Down Down Right Right Right"
app_press Down; sleep 0.3
app_press Down; sleep 0.3
app_press Right; sleep 0.3
app_press Right; sleep 0.3
app_press Right; sleep 0.3

monitor_announce RUNNING "Selecting chapter 14 (Enter)"
app_press Return; sleep 3

app_wait_for_chapter "EXO.14" && TEST_PASS=$((TEST_PASS+1)) || TEST_FAIL=$((TEST_FAIL+1))

# Step 1: Left-click to select/highlight a word
monitor_announce RUNNING "Clicking at (225, 200) to highlight a word"
app_click 225 200
sleep 2

if app_wait_for_log "Highlight saved:" 5; then
    echo "PASS: Highlight created"
    TEST_PASS=$((TEST_PASS+1))
else
    echo "FAIL: No highlight created"
    TEST_FAIL=$((TEST_FAIL+1))
fi

app_assert_no_crash && TEST_PASS=$((TEST_PASS+1)) || TEST_FAIL=$((TEST_FAIL+1))

# Step 2: Right-click at same position to open context menu
monitor_announce RUNNING "Right-clicking at (225, 200) to open context menu"
app_click 225 200 150 3
sleep 1

# Step 3: Click on "Del" button
# Context menu appears at (225, 200) with MENU_WIDTH=190, MENU_HEIGHT=32
# MENU_PADDING=4, DELETE_WIDTH=50
# "Del" rect: x=225+4=229, y=200+4=204, w=50, h=32-8=24
# Center of "Del": x=229+25=254, y=204+12=216
DEL_X=$((225 + 4 + 25))
DEL_Y=$((200 + 4 + 12))
monitor_announce RUNNING "Clicking 'Del' at ($DEL_X, $DEL_Y)"
app_click $DEL_X $DEL_Y
sleep 1

if app_wait_for_log "Highlight deleted:" 5; then
    echo "PASS: Highlight deleted via 'Del' button"
    TEST_PASS=$((TEST_PASS+1))
else
    echo "FAIL: Highlight was not deleted"
    TEST_FAIL=$((TEST_FAIL+1))
fi

app_assert_no_crash && TEST_PASS=$((TEST_PASS+1)) || TEST_FAIL=$((TEST_FAIL+1))

# Step 4: Right-click at same position again — should NOT open context menu
# (highlight was deleted, so HighlightAtWord should return nullptr)
monitor_announce RUNNING "Right-clicking again at (225, 200) — should NOT show menu"
app_click 225 200 150 3
sleep 1

# Click at same position — should NOT trigger any context menu action
# (menu was never shown because highlight was deleted)
app_click $DEL_X $DEL_Y
sleep 1

app_assert_no_crash && TEST_PASS=$((TEST_PASS+1)) || TEST_FAIL=$((TEST_FAIL+1))

app_stop
if [ -n "$TEST_MONITOR_BINARY" ]; then
    monitor_announce PASS "All context menu tests completed"
    sleep 1
fi

test_report
test_cleanup
