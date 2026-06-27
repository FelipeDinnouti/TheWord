# TheWord Test Monitor

A small raylib window that displays the current test step during
integration test runs. Shows a live status log alongside the app
being tested.

## Usage

```bash
# Build with the monitor
cmake -B build_monitor -DBUILD_TEST_MONITOR=ON -G "Unix Makefiles"
cmake --build build_monitor --parallel

# In one terminal: start the monitor
./build_monitor/test_monitor

# In another terminal: run an integration test that writes to
# /tmp/theword_test_status.txt
```

## Status File Protocol

The monitor reads `/tmp/theword_test_status.txt` every ~200ms.
Write one step per file update:

```
status:RUNNING
text:Sending G key (center menu)
```

| Field    | Values                      | Description                |
|----------|-----------------------------|----------------------------|
| `status:` | `RUNNING`, `PASS`, `FAIL`   | Current step outcome       |
| `text:`   | any string                  | Step description           |

Each step is shown in the current-step area (large, colored).\
When `text:` changes, the previous step is pushed to the history
log (up to 10 entries).

## Test Script Integration

```bash
announce() {
    local status="$1" text="$2"
    printf "status:%s\ntext:%s\n" "$status" "$text" > /tmp/theword_test_status.txt
}

announce RUNNING "Sending G key (center menu)"
xdotool keydown --window "$WIN_ID" g; sleep 0.15; xdotool keyup --window "$WIN_ID" g
announce PASS "Center menu opened"
```

## Window

- **Size**: 480 x 120 (adjustable in source)
- **Title**: "TheWord Test Monitor"
- **FPS**: 8 (low CPU usage, file-poll only)
