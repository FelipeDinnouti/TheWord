# Integration Testing with xdotool

> Automated GUI testing for TheWord using X11 automation and AddressSanitizer.

## Overview

TheWord is a raylib desktop app with no built-in test harness for UI flows. We test
navigation, screen transitions, and chapter loading by sending synthetic keyboard
and mouse events via **xdotool** while the app runs under **AddressSanitizer** (ASan)
to catch memory errors.

## Tools

| Tool | Purpose |
|------|---------|
| `xdotool` | Send keystrokes, mouse clicks, focus windows |
| `AddressSanitizer` (`-fsanitize=address`) | Detect use-after-free, buffer overflows |
| `Xvfb` (optional) | Virtual framebuffer for headless CI runs |
| `gdb` (optional) | Backtrace when ASan isn't enough |

## IMPORTANT: PID-Based Window Finding

**Never use `xdotool search --name "TheWord"`** — it does substring matching and will
match your terminal's title bar if it shows the project path (e.g. `~/Remote/TheWord`).
This silently sends all events to the wrong window.

**Always find the window by PID:**

```bash
./build_asan/theword &
APP_PID=$!
sleep 3
WIN_ID=$(xdotool search --pid "$APP_PID" 2>/dev/null | head -1)
```

## Workspace Management

If the app starts on a different X desktop, `windowactivate` will switch workspaces,
disrupting the user. Always move the window to the current desktop first:

```bash
CURRENT_DESKTOP=$(xdotool get_desktop)
xdotool set_desktop_for_window "$WIN_ID" "$CURRENT_DESKTOP"
xdotool windowfocus "$WIN_ID"
```

## Key Event Timing

**Use `keydown`/`keyup` separated by ≥100ms, not `key`.**

`xdotool key G` sends KeyPress+KeyRelease within milliseconds — both are processed
in the same `glfwPollEvents()` call inside raylib's `EndDrawing()`. The key is
pressed and released before `IsKeyPressed()` checks the state, so the press is
invisible to the app.

Correct approach:
```bash
xdotool keydown --window "$WIN_ID" g
sleep 0.15   # >1 frame at 60fps
xdotool keyup --window "$WIN_ID" g
sleep 1.5
```

## Cinnamon WM Conflicts

Cinnamon intercepts **Left/Right arrow keys** for workspace switching, even with
`--window`. These keys cannot be used for automated testing. Use mouse clicks on
the bottom bar buttons instead for prev/next chapter navigation, or navigate via
the center menu (G → Enter → select book → select chapter).

Safe keys that are not WM-intercepted:
- `g` (center menu)
- `s` (settings)
- `a` (about)
- `Return`, `Escape`, `Down`, `Up`
- Mouse clicks (with correct coordinates)

## Confirmation Checkpoints

### Output log patterns

The app prints startup logs to stdout. After navigation you can check for
specific log messages (`Logger::Info` at checkpoints):

```bash
# Run with output capture
./build_asan/theword > /tmp/theword.log 2>&1 &
APP_PID=$!

# ... send input ...

grep -q "Loaded chapter: GEN.2" /tmp/theword.log && echo "PASS"
```

**Important:** `Logger::Info` writes to `stdout`, not stderr. Capture with `>` not `2>`.
Output is fully buffered when redirected; the `fflush` in `WriteLog` ensures timely
delivery.

### Exit status / crash detection

```bash
if ! kill -0 $APP_PID 2>/dev/null; then
    wait $APP_PID
    echo "CRASHED: exit code $?"
fi
```

Exit code 139 = SIGSEGV. Under ASan, the report includes full backtrace.

### Screenshot diffing (optional)

```bash
import -window "$WIN_ID" /tmp/shot_after.png
compare -metric AE /tmp/baseline.png /tmp/shot_after.png /tmp/diff.png 2>&1
```

## Complete Test Script

```bash
#!/bin/bash
set -e
cd "$(dirname "$0")/.."

BUILD_DIR="${1:-build_asan}"
CURRENT_DESKTOP=$(xdotool get_desktop)
PASS=0; FAIL=0

check() {
    local label="$1" pattern="$2" logfile="$3"
    if grep -q "$pattern" "$logfile" 2>/dev/null; then
        echo "  PASS: $label"; ((PASS++))
    else
        echo "  FAIL: $label"; ((FAIL++))
    fi
}

# Build ASan
cmake -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_FLAGS="-fsanitize=address -fno-omit-frame-pointer" \
    -G "Unix Makefiles" 2>/dev/null
cmake --build "$BUILD_DIR" --parallel 2>/dev/null

LOGFILE=$(mktemp)
"$BUILD_DIR/theword" > "$LOGFILE" 2>&1 &
APP_PID=$!

while ! WIN_ID=$(timeout 1 xdotool search --pid "$APP_PID" 2>/dev/null); do
    if ! kill -0 $APP_PID 2>/dev/null; then wait $APP_PID; echo "CRASHED on startup"; exit 1; fi
    sleep 0.3
done
WIN_ID=$(echo "$WIN_ID" | head -1)

# Move to current desktop and focus
xdotool set_desktop_for_window "$WIN_ID" "$CURRENT_DESKTOP"
xdotool windowfocus "$WIN_ID"
sleep 3

check "GEN.1 loaded" "Loaded chapter: GEN.1" "$LOGFILE"

# Navigate via center menu: G → Enter (Books) → Down→Down (Exodus) → Enter
xdotool keydown --window "$WIN_ID" g;    sleep 0.15; xdotool keyup --window "$WIN_ID" g
sleep 1.5
xdotool keydown --window "$WIN_ID" Return; sleep 0.15; xdotool keyup --window "$WIN_ID" Return
sleep 1.5
xdotool keydown --window "$WIN_ID" Down;  sleep 0.15; xdotool keyup --window "$WIN_ID" Down
sleep 0.5
xdotool keydown --window "$WIN_ID" Down;  sleep 0.15; xdotool keyup --window "$WIN_ID" Down
sleep 0.5
xdotool keydown --window "$WIN_ID" Return; sleep 0.15; xdotool keyup --window "$WIN_ID" Return
sleep 2
xdotool keydown --window "$WIN_ID" Return; sleep 0.15; xdotool keyup --window "$WIN_ID" Return
sleep 2

check "Exodus 1 loaded" "Loaded chapter: EXO.1" "$LOGFILE"

kill $APP_PID 2>/dev/null; wait $APP_PID 2>/dev/null || true

if grep -q "AddressSanitizer\|ERROR: AddressSanitizer:" "$LOGFILE"; then
    echo "  FAIL: ASan error"; ((FAIL++))
else
    echo "  PASS: No ASan errors"; ((PASS++))
fi

echo "---"; echo "Results: $PASS passed, $FAIL failed"
rm -f "$LOGFILE"
```

## Mouse Clicks for Bottom Bar Navigation

Since Left/Right keys are intercepted by Cinnamon, use the bottom bar buttons
for prev/next chapter testing. The bottom bar is 50px tall at the window bottom:

```bash
GEO=($(xdotool getwindowgeometry "$WIN_ID" | grep -oP '\d+' | head -4))
WIN_W=${GEO[2]}
WIN_H=${GEO[3]}
BAR_CENTER_Y=$((WIN_H - 25))

# Prev button (left side)
xdotool mousemove --window "$WIN_ID" 25 $BAR_CENTER_Y
sleep 0.05
xdotool click --window "$WIN_ID" 1
sleep 2

# Next button (right side)
xdotool mousemove --window "$WIN_ID" $((WIN_W - 25)) $BAR_CENTER_Y
sleep 0.05
xdotool click --window "$WIN_ID" 1
sleep 2

# Center button (opens popup)
xdotool mousemove --window "$WIN_ID" $((WIN_W / 2)) $BAR_CENTER_Y
sleep 0.05
xdotool click --window "$WIN_ID" 1
sleep 2
```

Unlike `key`, mouse clicks with `mousemove` + `click` work correctly because
the click (ButtonPress + ButtonRelease) is a single atomic event that raylib
detects via `IsMouseButtonPressed()`.

## Virtual Framebuffer (CI / Headless)

For automated runs without a display:

```bash
export DISPLAY=:99
Xvfb :99 -screen 0 800x600x24 &
XVFB_PID=$!
bash test_integration.sh
kill $XVFB_PID 2>/dev/null
```

## Common Pitfalls

| Pitfall | Fix |
|---------|------|
| Keys go to terminal window | Use **PID-based** window search, not name-based |
| `xdotool key G` not detected | Use `keydown`/`keyup` with ≥100ms gap |
| Workspace switches | Move window to current desktop with `set_desktop_for_window` |
| LEFT/RIGHT intercepted by Cinnamon | Use mouse clicks on bottom bar, or center menu navigation |
| Coordinates miss buttons | Compute from window geometry, not fixed pixel values |
| Timing flakiness | Increase sleeps (1.5s after key events, 3s for startup) |
| ASan not catching bug | Also try `-fsanitize=undefined` for UB detection |
| App crash on startup | Check assets exist (`ls assets/usfm/*.usfm`) |
| Window not found | Use `xdotool search --pid $PID`; verify with `xdotool getwindowname $WIN_ID` |
