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
| Current status | `the-word-docs/04-planning/Progress Tracking.md` |
| Agent workflow | `the-word-docs/07-ai-collaboration/Agent Workflow.md` |

## Agent Workflow

1. **Read** — Find and read the relevant doc before writing code
2. **Plan** — Read Doc-First Checklist (`the-word-docs/07-ai-collaboration/Doc-First Checklist.md`)
3. **Implement** — Follow conventions below, write tests (doctest)
4. **Verify** — Build and run
5. **Document** — Update `the-word-docs/` with anything new

## Coding Conventions

- **Naming**: Classes = `PascalCase`, methods = `PascalCase`, variables = `camelCase`, constants = `SCREAMING_SNAKE_CASE`
- **Files**: One class per file (`ClassName.h` + `ClassName.cpp`), include guards (`#ifndef NAME_H`)
- **Dependencies**: Acyclic: `core → data → text → document → renderer`
- **No comments** unless explaining non-obvious logic
- **Commits**: Always use [Conventional Commits](https://www.conventionalcommits.org/) — `feat:`, `fix:`, `refactor:`, `docs:`, `test:`, `chore:`, etc.

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

## See Also

- `the-word-docs/06-ops/Build Guide.md` — Full build instructions
- `the-word-docs/02-architecture/Data Source Architecture.md` — Dual-source design
- `the-word-docs/05-reference/YouVersion API.md` — API details
- `the-word-docs/07-ai-collaboration/Convention Reference.md` — Full convention spec
