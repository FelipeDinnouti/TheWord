# Convention Reference

> Status: Active | Last Updated: 2026-06-26

## Code Conventions

### Naming
- **Classes**: `PascalCase` (e.g., `LayoutEngine`, `DocumentManager`)
- **Methods/Functions**: `PascalCase` (e.g., `LayoutChapter`, `GetVisibleSpans`)
- **Variables**: `camelCase` (e.g., `scrollY`, `currentChapter`)
- **Constants**: `SCREAMING_SNAKE_CASE` (e.g., `WINDOW_WIDTH`, `DEFAULT_BIBLE_ID`)
- **Namespaces**: `snake_case` nested under `theword::`:
  - `theword::core` — cross-cutting utilities (Platform, Config, Theme, Logger, IAssetProvider)
  - `theword::data` — data sources (ChapterProvider, USFMParser, BibleClient, CompositeProvider)
  - `theword::text` — layout engine
  - `theword::document` — document manager
  - `theword::highlight` — highlighting system (Highlighter, PersistenceInterface)
  - `theword::persistence` — SQLite operations
  - `theword::input` — input handling
  - `theword::renderer` — UI rendering (Renderer, UIManager, dialogs)
  - `theword::event` — event bus and event structs
  - `theword::app` — top-level application orchestrator
  - Flat constants namespaces: `theword::core::config`, `theword::core::key`, `theword::core::theme`
- **File-scope helpers**: Anonymous namespaces (`namespace { ... }`) instead of `static` keyword
- **Event handlers**: Methods named `On<EventName>` (e.g., `OnScroll`, `OnResize`)

### File Organization
- One class per file: `ClassName.h` + `ClassName.cpp`
- Header files in `src/<module>/`
- Include guards: `#ifndef NAME_H / #define NAME_H / #endif`
- No comments unless explaining non-obvious logic
- Keep public interfaces small and focused

### Includes
- Project headers use relative paths from `src/`: `"module/ClassName.h"`
- Standard library first, then Raylib, then project headers
- No circular includes

### Dependencies
- Keep acyclic: `core → data → text → document → renderer`
- No upward dependencies (renderer can depend on document, but not vice versa)
- Cross-cutting communication flows through the event bus, not direct method calls
  across layers
- Platform abstractions live in `core/` — `IAssetProvider` for file I/O,
  `IHttpClient` for HTTP, `Platform.h` for init/dispatch

### Platform Abstraction Pattern
- Define an abstract interface in `core/` (e.g., `IAssetProvider`)
- Implement per-platform in separate `.cpp` files, compiled conditionally
- Create a factory or dispatch function in `Platform.cpp` that returns the correct
  implementation based on `#ifdef` guards
- The goal: zero `#ifdef` blocks in business logic code (`main.cpp`, `UIManager.cpp`, etc.)

### Ownership
- Prefer `std::unique_ptr` for owning pointers
- Prefer references (`&`) for non-owning access
- Prefer `std::vector` for owned collections
- No raw `new`/`delete` — use `std::make_unique` or `std::make_shared`
- The `App` class owns all top-level subsystems; no global singletons

## Project Structure

```
src/
├── app/                 # Application orchestrator (App class)
├── main.cpp             # Entry point, creates App and calls Run()
├── core/                # Cross-cutting utilities (Config, Theme, Platform, IAssetProvider, EnvLoader, etc.)
├── data/                # Data layer (ChapterProvider, USFMParser, BibleClient, CompositeProvider)
├── text/                # Layout engine
├── document/            # Document manager
├── highlight/           # Highlighting system
├── persistence/         # SQLite operations
├── input/               # Input handling
├── renderer/            # UI rendering (Renderer, UIManager, dialogs)
└── event/               # Event bus and event structs

assets/                   # Fonts, JSON data, USFM files, shaders
tests/                    # Unit tests (doctest)
scripts/                  # Build scripts (build-android.sh, build-wasm.sh)
```

## Documentation Structure

```
the-word-docs/
├── 00-INDEX.md           # Always read this first
├── 01-vision/            # Why it exists
├── 02-architecture/      # How it works
├── 03-modules/           # Module specs (one per module)
├── 04-planning/          # Project management
├── 05-reference/         # External references
├── 06-ops/               # Build and operations
└── 07-ai-collaboration/  # Agent workflows
```

## Branch Naming

- `feat/<name>` — New features
- `fix/<name>` — Bug fixes
- `docs/<name>` — Documentation changes
- `refactor/<name>` — Code restructuring

## Commit Message Style

```
type: concise description

Optional body with details.
```

Types: `feat`, `fix`, `docs`, `refactor`, `test`, `chore`

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
  git tag -am "$(grep -oP 'VERSION \K[0-9.]+' CMakeLists.txt)" "v$(grep -oP 'VERSION \K[0-9.]+' CMakeLists.txt)"
  ```
- **Pre-release versions**: Use SemVer suffixes (e.g. `1.5.0-beta.1`)
  for test releases — set them in CMakeLists.txt and tag as-is.
- **Generated header**: `Version.h` is auto-generated from `src/core/Version.h.in` via `configure_file()`. Reconfigure to pick up version changes.
- **Runtime access**: `#include "Version.h"` → `theword::core::APP_VERSION` (string), `APP_VERSION_MAJOR`/`MINOR`/`PATCH` (int)
