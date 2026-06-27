#!/bin/bash
# ============================================================================
# TheWord Integration Test Helpers
# ============================================================================
# AI-friendly action library for GUI integration testing.
# Source this file:  source scripts/test_helpers.sh
#
# Each function does one thing, returns 0 on success / 1 on failure,
# and prints clear PASS/FAIL lines parsable by AI agents.
#
# Usage:
#   source scripts/test_helpers.sh
#   test_init
#   app_start build_asan/theword
#   app_wait_for_chapter GEN.1 && TEST_PASS=$((TEST_PASS+1)) ...
#   test_report
#   test_cleanup
# ============================================================================

# ── Globals ──────────────────────────────────────────────────────────────────
TEST_APP_PID=""
TEST_APP_WIN=""
TEST_APP_LOGFILE=""
TEST_MONITOR_PID=""
TEST_MONITOR_WIN=""
TEST_MONITOR_BINARY=""
TEST_CURRENT_DESKTOP=""

# ── Lifecycle ────────────────────────────────────────────────────────────────

# test_init - create temp files, register cleanup trap
test_init() {
    TEST_PASS=0
    TEST_FAIL=0
    TEST_CURRENT_DESKTOP=$(xdotool get_desktop 2>/dev/null)
    TEST_APP_LOGFILE=$(mktemp /tmp/theword_test_log.XXXXXX 2>/dev/null)
    trap 'test_cleanup' EXIT INT TERM
}

# test_cleanup - kill leftover processes, remove temp files
test_cleanup() {
    [ -n "$TEST_APP_PID" ] && kill "$TEST_APP_PID" 2>/dev/null || true
    [ -n "$TEST_MONITOR_PID" ] && kill "$TEST_MONITOR_PID" 2>/dev/null || true
    wait "$TEST_APP_PID" 2>/dev/null || true
    wait "$TEST_MONITOR_PID" 2>/dev/null || true
    [ -n "$TEST_APP_LOGFILE" ] && rm -f "$TEST_APP_LOGFILE" 2>/dev/null || true
}

# test_report - print summary
test_report() {
    echo "---"
    echo "Results: $TEST_PASS passed, $TEST_FAIL failed"
    [ "$TEST_FAIL" -eq 0 ]
}

# ── App management ──────────────────────────────────────────────────────────

# app_start <binary> - launch app, move to current desktop, focus
# Returns window ID on success, exits on failure
app_start() {
    local binary="$1"
    if [ -z "$binary" ]; then
        echo "FAIL: app_start requires binary path"
        return 1
    fi
    if [ ! -x "$binary" ]; then
        echo "FAIL: binary not found or not executable: $binary"
        return 1
    fi

    "$binary" > "$TEST_APP_LOGFILE" 2>&1 &
    TEST_APP_PID=$!

    local timeout=10
    while [ $timeout -gt 0 ]; do
        sleep 0.5
        TEST_APP_WIN=$(xdotool search --pid "$TEST_APP_PID" 2>/dev/null | head -1)
        [ -n "$TEST_APP_WIN" ] && break
        timeout=$((timeout - 1))
        if ! kill -0 "$TEST_APP_PID" 2>/dev/null; then
            wait "$TEST_APP_PID" 2>/dev/null || true
            echo "FAIL: app_start crashed on launch"
            return 1
        fi
    done

    if [ -z "$TEST_APP_WIN" ]; then
        echo "FAIL: app_start window not found after 5s"
        return 1
    fi

    xdotool set_desktop_for_window "$TEST_APP_WIN" "$TEST_CURRENT_DESKTOP" 2>/dev/null
    xdotool windowfocus "$TEST_APP_WIN" 2>/dev/null
    echo "$TEST_APP_WIN"
}

# app_stop - kill the app
app_stop() {
    if [ -n "$TEST_APP_PID" ]; then
        kill "$TEST_APP_PID" 2>/dev/null || true
        wait "$TEST_APP_PID" 2>/dev/null || true
        TEST_APP_PID=""
    fi
}

# ── Input simulation ────────────────────────────────────────────────────────

# app_press <key> [intra_delay_ms] - keydown/wait/keyup (default delay 150ms)
# Does NOT sleep after; caller decides inter-action timing.
app_press() {
    local key="$1"
    local delay="${2:-150}"
    if [ -z "$TEST_APP_WIN" ]; then
        echo "FAIL: app_press no app window (call app_start first)"
        return 1
    fi
    xdotool windowfocus "$TEST_APP_WIN" 2>/dev/null || true
    xdotool keydown --window "$TEST_APP_WIN" "$key" 2>/dev/null || return 1
    sleep "$(echo "scale=3; $delay / 1000" | bc -l 2>/dev/null || echo "0.15")"
    xdotool keyup --window "$TEST_APP_WIN" "$key" 2>/dev/null || return 1
}

# app_click <x> <y> - mouse click at window-relative coordinates
app_click() {
    local x="$1" y="$2"
    if [ -z "$TEST_APP_WIN" ]; then
        echo "FAIL: app_click no app window"
        return 1
    fi
    xdotool mousemove --window "$TEST_APP_WIN" "$x" "$y" 2>/dev/null || return 1
    sleep 0.05
    xdotool click --window "$TEST_APP_WIN" 1 2>/dev/null || return 1
}

# ── Assertions / waits ──────────────────────────────────────────────────────

# app_wait_for_log <pattern> [timeout_secs] - poll app log for pattern
# Returns 0 if found, 1 if timeout
app_wait_for_log() {
    local pattern="$1"
    local timeout="${2:-15}"
    local waited=0
    if [ -z "$TEST_APP_LOGFILE" ]; then
        echo "FAIL: app_wait_for_log no logfile"
        return 1
    fi
    while [ $waited -lt "$timeout" ]; do
        if grep -q "$pattern" "$TEST_APP_LOGFILE" 2>/dev/null; then
            return 0
        fi
        sleep 0.5
        waited=$((waited + 1))
    done
    return 1
}

# app_wait_for_chapter <chapter_id> [timeout_secs] - wait for chapter to load
app_wait_for_chapter() {
    local chapter="$1"
    local timeout="${2:-15}"
    if app_wait_for_log "Loaded chapter: $chapter" "$timeout"; then
        echo "PASS: $chapter loaded"
        return 0
    else
        echo "FAIL: $chapter did not load within ${timeout}s"
        return 1
    fi
}

# app_assert_no_crash - check that app is still alive and no ASan errors
app_assert_no_crash() {
    local ok=0
    if ! kill -0 "$TEST_APP_PID" 2>/dev/null; then
        echo "FAIL: app crashed"
        ok=1
    fi
    if [ -f "$TEST_APP_LOGFILE" ] && \
       grep -q "AddressSanitizer\|ERROR: AddressSanitizer" "$TEST_APP_LOGFILE" 2>/dev/null; then
        echo "FAIL: ASan error detected"
        ok=1
    fi
    return $ok
}

# ── Screenshot ──────────────────────────────────────────────────────────────

# app_screenshot <file> - capture window content to PNG
# Uses ImageMagick's 'import' if available; fail silently otherwise.
app_screenshot() {
    local file="$1"
    if [ -z "$TEST_APP_WIN" ]; then
        echo "FAIL: app_screenshot no app window"
        return 1
    fi
    if command -v import &>/dev/null; then
        import -window "$TEST_APP_WIN" "$file" 2>/dev/null && \
            echo "PASS: screenshot saved to $file" || \
            echo "WARN: screenshot failed"
    else
        echo "WARN: screenshot skipped (ImageMagick 'import' not found)"
    fi
}

# ── Test Monitor ────────────────────────────────────────────────────────────

# monitor_start - launch the test monitor window (if binary is configured)
monitor_start() {
    if [ -z "$TEST_MONITOR_BINARY" ] || [ ! -x "$TEST_MONITOR_BINARY" ]; then
        echo "WARN: monitor_start skipped (set TEST_MONITOR_BINARY)"
        return 0
    fi
    "$TEST_MONITOR_BINARY" &
    TEST_MONITOR_PID=$!
    local timeout=5
    while [ $timeout -gt 0 ]; do
        sleep 0.3
        TEST_MONITOR_WIN=$(xdotool search --pid "$TEST_MONITOR_PID" 2>/dev/null | head -1)
        [ -n "$TEST_MONITOR_WIN" ] && break
        timeout=$((timeout - 1))
    done
    if [ -n "$TEST_MONITOR_WIN" ]; then
        xdotool set_desktop_for_window "$TEST_MONITOR_WIN" "$TEST_CURRENT_DESKTOP" 2>/dev/null
        local screen_w
        screen_w=$(xdotool getdisplaygeometry 2>/dev/null | awk '{print $1}')
        xdotool windowmove "$TEST_MONITOR_WIN" $((screen_w / 2 - 240)) 0 2>/dev/null || true
    fi
}

# monitor_announce <status> <text> - write step to status file
monitor_announce() {
    printf "status:%s\ntext:%s\n" "$1" "$2" > /tmp/theword_test_status.txt
}
