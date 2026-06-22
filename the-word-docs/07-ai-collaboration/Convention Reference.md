# Convention Reference

> Status: Active | Last Updated: 2026-06-21

## Code Conventions

### Naming
- **Classes**: `PascalCase` (e.g., `LayoutEngine`, `DocumentManager`)
- **Methods/Functions**: `PascalCase` (e.g., `layoutChapter()`, `getVisibleSpans()`)
- **Variables**: `camelCase` (e.g., `scrollY`, `currentChapter`)
- **Constants**: `SCREAMING_SNAKE_CASE` (e.g., `WINDOW_WIDTH`, `DEFAULT_BIBLE_ID`)
- **Namespaces**: `snake_case` (e.g., `config`)

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

## Project Structure

```
src/
├── main.cpp              # Entry point, render loop
├── core/                 # Cross-cutting utilities
├── data/                 # Data layer (API, USFM, persistence)
├── text/                 # Layout engine
├── document/             # Document manager
├── highlight/            # Highlighting system
├── input/                # Input handling
├── persistence/          # SQLite operations
└── renderer/             # UI rendering

assets/                   # Fonts, JSON data, shaders
tests/                    # Unit tests (doctest)
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
