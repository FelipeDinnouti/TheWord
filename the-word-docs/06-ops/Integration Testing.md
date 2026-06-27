# Integration Testing with xdotool

> Automated GUI testing for TheWord using X11 automation and AddressSanitizer.

## Overview

TheWord is a raylib desktop app with no built-in test harness for UI flows. We test
navigation, screen transitions, and chapter loading by sending synthetic keyboard
and mouse events via **xdotool** while the app runs under **AddressSanitizer** (ASan)
to catch memory errors.

## Quick Start

```bash
# Run the example test (without monitor):
./scripts/test_integration.sh

# Run with the live test monitor:
TEST_MONITOR_BINARY=build_monitor/tools/test_monitor/test_monitor \
    ./scripts/test_integration.sh
```

## Action Library

The file `scripts/test_helpers.sh` is a **bash action library** designed for AI agents
that cannot see the screen. Every function returns a parseable exit code and prints
PASS/FAIL lines. Source it in any script:

```bash
source scripts/test_helpers.sh
```

### Lifecycle

```bash
test_init        # Create temp files, register cleanup trap
test_cleanup     # Kill processes, remove temp files
test_report      # Print "Results: X passed, Y failed"
```

### App Management

```bash
app_start build_asan/theword   # Launch → find window → focus → move to desktop
app_stop                       # Graceful kill
```

### Input Simulation

```bash
app_press g                    # KeyDown → 150ms wait → KeyUp
app_press Return 200           # Custom intra-key delay (ms)
app_click 200 150              # Mouse click at window-relative (x, y)
```

**Key timing detail:** `xdotool key G` sends KeyPress+KeyRelease within
milliseconds — both are processed in the same `glfwPollEvents()`, making the
press invisible to `IsKeyPressed()`. `app_press` splits into `keydown`/`keyup`
with a configurable delay (default 150ms ≥ 1 frame at 60fps).

**Mouse timing detail:** Same issue — `xdotool click` sends ButtonPress+ButtonRelease
within milliseconds. `app_click` splits into `mousedown`/`mouseup` with a
configurable hold time (default 150ms).

### Assertions / Waits

```bash
app_wait_for_log "some pattern" [timeout=15]   # Poll stdout log for regex
app_wait_for_chapter "GEN.1" [timeout=15]      # Wait for "Loaded chapter: GEN.1"
app_assert_no_crash                             # Check PID alive + no ASan errors
```

### Screenshots

```bash
app_screenshot /tmp/shot.png    # Requires ImageMagick 'import'
```

### Test Monitor

```bash
monitor_start                   # Launch test monitor window (if binary configured)
monitor_announce RUNNING "Step description"
monitor_announce PASS "Step completed"
```

Set `TEST_MONITOR_BINARY` to the path of a `test_monitor` build.

## Complete Example Script

```bash
#!/bin/bash
source scripts/test_helpers.sh

test_init
app_start build_asan/theword
sleep 3

app_wait_for_chapter "GEN.1"

app_press g; sleep 1.5
app_press Return; sleep 1.5
app_press Down; sleep 0.5
app_press Return; sleep 2
app_press Return; sleep 2

app_wait_for_chapter "EXO.1"
app_assert_no_crash

test_report
test_cleanup
```

## Tools Reference

| Tool | Purpose |
|------|---------|
| `xdotool` | Send keystrokes, mouse clicks, focus windows |
| `AddressSanitizer` (`-fsanitize=address`) | Detect use-after-free, buffer overflows |
| `Xvfb` (optional) | Virtual framebuffer for headless CI runs |
| `gdb` (optional) | Backtrace when ASan isn't enough |
| `test_monitor` (`BUILD_TEST_MONITOR`) | Live status window for test steps |

## Safe Keys

The following keys work reliably with `app_press` (using keydown/keyup split):
- `g` (center menu), `s` (settings), `a` (about)
- `Return`, `Escape`, `Down`, `Up`, `Left`/`Right`
- `1`-`9` (chapter shortcuts)
- Mouse clicks via `app_click` (mousedown/mouseup split)

Left/Right arrows were previously flagged as intercepted by Cinnamon, but the
workspace-switching issue was caused by key events going to the wrong X window.
PID-based window targeting (`--window` flag) resolves this. If you still observe
workspace switching, verify that `TEST_APP_WIN` points to the correct window.

## Virtual Framebuffer (CI / Headless)

For automated runs without a display:

```bash
export DISPLAY=:99
Xvfb :99 -screen 0 800x600x24 &
XVFB_PID=$!
bash scripts/test_integration.sh
kill $XVFB_PID 2>/dev/null
```

## Common Pitfalls

| Pitfall | Fix |
|---------|------|
| Keys go to terminal window | `app_start` uses PID-based search (not name-based) |
| `xdotool key G` not detected | Use `app_press` (keydown/keyup split) |
| Workspace switches | `app_start` moves window to current desktop |
| Workspace switches on arrow keys | Verify `TEST_APP_WIN` is the correct PID-owned window |
| Coordinates miss buttons | Compute from window geometry |
| Timing flakiness | Increase sleeps between actions |
| ASan not catching bug | Also try `-fsanitize=undefined` for UB detection |
| App crash on startup | Check assets (`ls assets/usfm/*.usfm`) |
| Window not found | Verify with `xdotool getwindowname $WIN_ID` |
