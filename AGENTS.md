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
| Architecture questions | `the-word-docs/02-architecture/` |
| Data source design | `the-word-docs/02-architecture/Data Source Architecture.md` |
| Module details | `the-word-docs/03-modules/<Module>.md` |
| Current release | `the-word-docs/04-planning/Release Plan.md` |
| Current status | `the-word-docs/04-planning/Progress Tracking.md` |
| Agent workflow | `the-word-docs/07-ai-collaboration/Agent Workflow.md` |
| Font rendering | `the-word-docs/05-reference/Raylib Notes.md#crisp-font-rendering--directives` |

## Agent Workflow

1. **Read** — Find and read the relevant doc before writing code
2. **Plan** — Check `the-word-docs/04-planning/Release Plan.md` for current release scope; read Doc-First Checklist (`the-word-docs/07-ai-collaboration/Doc-First Checklist.md`)
3. **Write Plan** — Persist the implementation plan to `.opencode/plans/<release>.md` with checklist items; update it throughout development as progress is made
4. **Implement** — Follow conventions below, write tests (doctest)
5. **Verify** — Build and run
6. **Document** — Update `the-word-docs/` with anything new
7. **Version** — Bump version in `CMakeLists.txt` and tag (see [Versioning](#versioning) below)

## Coding Conventions

- **Naming**: Classes = `PascalCase`, methods = `PascalCase`, variables = `camelCase`, constants = `SCREAMING_SNAKE_CASE`
- **Files**: One class per file (`ClassName.h` + `ClassName.cpp`), include guards (`#ifndef NAME_H`)
- **Dependencies**: Acyclic: `core → data → text → document → renderer`
- **No comments** unless explaining non-obvious logic
- **Commits**: Always use [Conventional Commits](https://www.conventionalcommits.org/) — `feat:`, `fix:`, `refactor:`, `docs:`, `test:`, `chore:`, etc.
  1. `git add` only intended files (check `git status` first)
  2. `git commit -m "type: short description (max 72 chars)"`
  3. Include scope if helpful: `fix(reader): prevent > arrow overflow on mobile`

## Build Commands

```bash
# Configure (Linux)
cmake -B build -DCMAKE_BUILD_TYPE=Release -G "Unix Makefiles"

# Build
cmake --build build --parallel

# Run
./build/theword

# Full reconfigure (after adding .cpp files)
rm -rf build && cmake -B build -DCMAKE_BUILD_TYPE=Release -G "Unix Makefiles"
```

## CMake Caveats

1. `project(theword C CXX)` — must include `CXX`
2. Use `-G "Unix Makefiles"` on Linux (no ninja)
3. `find_package(CURL)` must come AFTER `FetchContent_MakeAvailable(raylib)` (only if using online API)
4. Delete `build/` and reconfigure when adding new `.cpp` files
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
- `the-word-docs/06-ops/Build Guide.md` — Full build instructions
- `the-word-docs/02-architecture/Data Source Architecture.md` — Dual-source design
- `the-word-docs/05-reference/YouVersion API.md` — API details
- `the-word-docs/07-ai-collaboration/Convention Reference.md` — Full convention spec
- `the-word-docs/06-ops/Integration Testing.md` — Full integration testing reference
- `scripts/test_helpers.sh` — Action library for AI agents
- `scripts/test_integration.sh` — Example test runner
