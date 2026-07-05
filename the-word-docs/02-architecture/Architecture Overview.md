# Architecture Overview

> Status: Updated 2026-06-22

## Four-Layer Architecture

The system is organized into four conceptual layers. Each layer builds on the one below it and is implemented as a distinct module within `src/`.

```
┌─────────────────────────────────────────────────┐
│                 UI LAYER                         │
│  Renderer · InputHandler · UIManager             │
│  (reads input, draws text, manages UI)           │
├─────────────────────────────────────────────────┤
│             DOCUMENT MANAGER                     │
│  DocumentManager                                 │
│  (infinite scroll, chapter lifecycle, anchors)   │
├─────────────────────────────────────────────────┤
│            TEXT LAYOUT ENGINE                    │
│  LayoutEngine                                    │
│  (segment-aware layout, word wrapping, caching)  │
├─────────────────────────────────────────────────┤
│                DATA LAYER                        │
│  ChapterProvider (interface)                     │
│    ├── USFMParser   (offline USFM files)         │
│    └── BibleClient  (online HTML API)            │
│  PersistenceManager (SQLite)                     │
└─────────────────────────────────────────────────┘
```

## Module Dependency Graph

```
main.cpp
  ├── Renderer
  │     └── UIManager      → Config
  ├── InputHandler        → DocumentManager, Highlighter, LayoutEngine
  └── DocumentManager
        ├── LayoutEngine
        │     └── Data Structures (Word, Span, Line, Segment, ChapterData)
        └── ChapterProvider (interface)
              ├── USFMParser
              └── BibleClient → Config

Highlighter
  └── PersistenceInterface → PersistenceManager → Config
```

All dependencies flow downward. No module depends on a higher layer.

## Module Responsibilities

| Module | Directory | Responsibility |
|--------|-----------|----------------|
| Config | `core/` | Constants: window size, asset paths, source selection |
| APIClient | `core/` | HTTP client wrapper (libcurl, hard dependency) |
| EnvLoader | `core/` | `.env` file parser (for API key) |
| USFMParser | `data/` | USFM file → ChapterData (Segment[] + Word[]) |
| BibleClient | `data/` | HTML API → ChapterData (Segment[] + Word[]) |
| ChapterProvider | `data/` | Abstract interface for both data sources |
| LayoutEngine | `text/` | Segment-aware layout, word wrapping, span generation |
| DocumentManager | `document/` | Infinite scroll, chapter lifecycle (talks to ChapterProvider) |
| Highlighter | `highlight/` | Per-word highlight management |
| PersistenceManager | `persistence/` | SQLite operations |
| InputHandler | `input/` | Input event translation (scroll, highlight selection, window resize) |
| Renderer | `renderer/` | Top-level render coordinator |
| UIManager | `renderer/` | Top bar, settings, context menu, dialogs |

## Key Design Properties

1. **Acyclic dependencies**: No circular includes or dependency cycles.
2. **Dual data sources**: ChapterProvider interface abstracts USFM (offline) and API (online). DocumentManager doesn't know which is active.
3. **Rich text model**: ChapterData carries both a flat Word[] array and a Segment[] array describing headings, poetry, and paragraph structure.
4. **Coordinate spaces**: Two spaces — Document (entire Bible) and Screen (viewport). Conversion is explicit and localized.
5. **Global word IDs**: Every word has a unique sequential ID across the entire Bible. This makes highlighting resolution-independent.
6. **Layout caching**: ChapterLayouts are cached by chapter ID. Cache is invalidated on font/width changes.
7. **Anchor-fixed prepend**: When prepending content, scroll position is adjusted so visible text does not jump.

## Cross-Cutting Concerns

### 1. Highlight + Persistence are tightly coupled

The highlighting system and persistence layer share the same data model. `Highlight` structs contain word ID ranges that must match exactly what SQLite stores. **Plan these together** — define the `Highlight` data format with the SQLite schema in mind, so the data format is consistent from day one.

**If highlighting is implemented first:**
- Use a `PersistenceInterface` (abstract base class) rather than hardcoding SQLite calls.
- This allows Phase 8 to implement the interface without changing highlight logic.

**If persistence is implemented first:**
- Define the `Highlight` struct that the highlighter reuses.
- Start with just the schema and data access; the UI for creating highlights comes later.

### 2. Renderer must be extracted from main.cpp

The current render loop lives entirely in `main.cpp`. This is unsustainable as features are added (highlights, rich text, UI controls, touch input).

**When Phase 4 begins, the renderer should be extracted:**
1. Create `src/renderer/Renderer.h/cpp` with a `Renderer` class that owns two fonts (`bodyFont`, `headingFont`), scroll state, and draw calls.
2. `main.cpp` becomes a thin orchestrator: init subsystems, call `Renderer::UpdateAndDraw()` in the loop, cleanup.
3. The `InputHandler` should feed events to the renderer rather than the renderer polling directly.

### 3. Android build (working, in polish phase)

Android NDK support is implemented. The build uses:
- A CMake toolchain file (`android.toolchain.cmake`) via the NDK
- `scripts/build-android.sh` for one-step build → APK packaging → signing
- `AndroidManifest.xml` with `NativeActivity` entry point
- `AndroidClient.cpp` (HTTP via native APIs) and `AndroidAssetProvider.cpp` (fonts via `AAssetManager`)
- DPI scaling via `AConfiguration_getDensity`
- Touch gesture system (`InputHandler` handles touch scroll, press FSM, pinch)

**Current limitations (Phase 10 in progress):**
- Only x86_64 ABI is built by the script (no arm64-v8a)
- No Java activity stub (IME for go-to dialog, native splash)
- Lifecycle save/restore not yet implemented
- Keyboard input requires a patch to raylib (`patches/raylib-android-keycodes.patch`)
- WASM persistence not yet wired (IDBFS)

**See:** `04-planning/Progress Tracking.md#phase-10--mobileandroid-in-progress`



