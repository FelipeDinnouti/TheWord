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
