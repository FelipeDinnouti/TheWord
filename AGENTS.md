# TheWord — Agent Development Guide

**TheWord** is a minimalist Bible study app (Raylib + C++17, dual-source data: USFM offline + HTML API online).

## Quick Start

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -G "Unix Makefiles"
cmake --build build --parallel
./build/theword
```

## Before You Code — Mandatory Reading

| Task | Read First |
|------|------------|
| Any task | `the-word-docs/00-INDEX.md` |
| Project state | `memory/State.md` |
| High-level roadmap | `the-word-docs/04-planning/Roadmap.md` |
| Current release scope | `the-word-docs/04-planning/Release Plan.md` |
| Active implementation checklist | `memory/Active.md` |
| Agent workflow | `the-word-docs/07-ai-collaboration/Agent Workflow.md` |
| Architecture questions | `the-word-docs/02-architecture/` |
| Data source design | `the-word-docs/02-architecture/Data Source Architecture.md` |
| Module details | `the-word-docs/03-modules/<Module>.md` |
| Coding conventions | `the-word-docs/07-ai-collaboration/Convention Reference.md` |
| Font rendering | `the-word-docs/05-reference/Raylib Notes.md#crisp-font-rendering--directives` |

### Doc Hierarchy

```
the-word-docs/04-planning/Roadmap.md    ← High-level version timeline (stable)
the-word-docs/04-planning/Release Plan.md ← Feature scope for current minor (stable)
memory/Active.md                         ← Granular implementation checklist (ephemeral)
memory/State.md                          ← Current project snapshot (per-session)
```

## Agent Workflow

See `the-word-docs/07-ai-collaboration/Agent Workflow.md` for the canonical workflow.
Implementation checklists live in `memory/Active.md` — update as tasks progress.

## Coding Conventions

See `the-word-docs/07-ai-collaboration/Convention Reference.md` for full conventions.
Key points: PascalCase classes/methods, camelCase variables, acyclic `core → data → text → document → renderer` dependencies, no comments unless non-obvious, [Conventional Commits](https://www.conventionalcommits.org/).

## Build Commands

```bash
# Build (Linux desktop)
./scripts/build-linux.sh                          # Release
./scripts/build-linux.sh --debug                  # Debug
./scripts/build-linux.sh --clean                  # Full reconfigure
./scripts/build-linux.sh --test                   # Build + run tests

# Cross-compile for Windows (from Linux)
./scripts/build-windows.sh

# Build for Android
./scripts/build-android.sh arm64-v8a              # Physical device
./scripts/build-android.sh x86_64                 # Emulator

# Build for WebAssembly
./scripts/build-wasm.sh

# Run
./build/theword
```

## CMake Caveats

1. `project(theword C CXX)` — must include `CXX`
2. Use `-G "Unix Makefiles"` on Linux (no ninja)
3. `find_package(CURL)` must come AFTER `FetchContent_MakeAvailable(raylib)` (only if using online API)
4. Delete `build/` and reconfigure when adding new `.cpp` files (or use `./scripts/build-linux.sh --clean`)
5. Linux links: `m pthread dl rt X11 Xcursor Xrandr Xi`

## Common Issues

| Issue | Solution |
|-------|----------|
| "CMAKE_CXX_COMPILE_OBJECT not set" | Add `CXX` to `project()` |
| New .cpp not compiled | `rm -rf build && cmake -B build ...` |
| libcurl not found | Install `libcurl4-openssl-dev` (optional, only for online API) |
| No USFM files | Download Bíblia Livre from ebible.org and place in `assets/usfm/` |
| API "Access denied" | Use Bible ID 3034 (BSB), not 111 (NIV) |

## Versioning

- **Source of truth**: `project(theword VERSION X.Y.Z)` in `CMakeLists.txt`
- **Scheme**: Standard SemVer `MAJOR.MINOR.PATCH` where:
  - `MAJOR` — big milestone releases (rarely bumped, e.g. 1→2)
  - `MINOR` — new features or system implementations (`feat:` commits)
  - `PATCH` — bug fixes, refactors, polish (`fix:`, `refactor:` commits)
- **When to bump**: Only at release time, not per-commit.
  Bump → reconfigure → build → test → tag → distribute.
- **Tagging**: Annotated tag after bumping and building:
  ```bash
  cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build --parallel
  git tag -am "$(grep -oP 'VERSION \K[0-9.]+' CMakeLists.txt)" "v$(grep -oP 'VERSION \K[0-9.]+' CMakeLists.txt)"
  ```
- **Pre-release versions**: Use SemVer suffixes (e.g. `1.5.0-alpha.1`)
  for test releases — set them in CMakeLists.txt and tag as-is.
- Generated `Version.h` is updated automatically on reconfigure (`cmake -B build ...`)
- Version is displayed in the About overlay and accessible at runtime via `theword::core::APP_VERSION`

## Integration Testing (AI Verification)

AI agents cannot see the screen. Use `scripts/test_helpers.sh` — a bash action
library for GUI integration testing. Every function returns a parseable exit
code and prints PASS/FAIL:

```bash
source scripts/test_helpers.sh

test_init
app_start build_asan/theword
sleep 3                                    # wait for chapter to load
app_wait_for_chapter "GEN.1" && PASS=$((PASS+1))

app_press g ; sleep 1.5                    # open center menu
app_press Return ; sleep 1.5               # select Books
app_press Down ; sleep 0.5                 # select Exodus
app_press Return ; sleep 2                 # open chapter grid
app_press Return ; sleep 2                 # select EXO.1

app_wait_for_chapter "EXO.1" && PASS=$((PASS+1))
app_assert_no_crash && PASS=$((PASS+1))

test_report
test_cleanup
```

| Function | Purpose |
|----------|---------|
| `test_init` / `test_cleanup` | Lifecycle (temp files, traps) |
| `app_start <binary>` | Launch app, find window by PID, focus |
| `app_stop` | Kill app |
| `app_press <key> [delay_ms]` | KeyDown + wait + KeyUp (handles raylib timing) |
| `app_click <x> <y> [hold_ms] [button]` | Mouse click at window-relative coords (split mousedown/mouseup, default button 1, 3=right) |
| `app_wait_for_chapter <id>` | Poll log until "Loaded chapter: <id>" appears |
| `app_assert_no_crash` | Check app alive + no ASan errors |
| `app_screenshot <file>` | Capture window to PNG (requires ImageMagick) |
| `monitor_start` | Launch live test monitor (requires `BUILD_TEST_MONITOR`) |
| `monitor_announce <status> <text>` | Write to monitor status file |

For a full example see `scripts/test_integration.sh`.

```bash
# Run the example test (without monitor):
./scripts/test_integration.sh

# Run with monitor:
TEST_MONITOR_BINARY=build_monitor/tools/test_monitor/test_monitor \
    ./scripts/test_integration.sh
```

## See Also

- `the-word-docs/04-planning/Release Plan.md` — Current release scope and backlog
- `the-word-docs/04-planning/Roadmap.md` — High-level version timeline
- `the-word-docs/06-ops/Build Guide.md` — Full build instructions
- `the-word-docs/02-architecture/Data Source Architecture.md` — Dual-source design
- `the-word-docs/05-reference/YouVersion API.md` — API details
- `the-word-docs/07-ai-collaboration/Convention Reference.md` — Full convention spec
- `the-word-docs/06-ops/Integration Testing.md` — Full integration testing reference
- `scripts/test_helpers.sh` — Action library for AI agents
- `scripts/test_integration.sh` — Example test runner
