# Architecture Overview

> Status: Updated 2026-07-18

## Module Architecture

The system is organized into logical layers. Each layer builds on the one below it and is implemented as a distinct module within `src/`.

## Layer Diagram

```
┌──────────────────────────────────────────────────────┐
│                    APP LAYER                          │
│  App.cpp (thin orchestrator, wires subsystems)       │
├──────────────────────────────────────────────────────┤
│                    UI LAYER                           │
│  ├── renderer/   Renderer · UIManager · RadialMenu   │
│  └── ui/         Screen-based UI                     │
│       ├── Screen (base class)                        │
│       ├── NavigationStack (push/pop screens)         │
│       ├── ReaderScreen, BookListScreen,              │
│       │   ChapterGridScreen, SettingsScreen,         │
│       │   HighlightBrowserScreen, CenterMenu,        │
│       │   CreditsOverlay, FontDiagnostic             │
│       ├── TapDetector (touch/click helper)           │
│       └── components (shared widgets)                │
├──────────────────────────────────────────────────────┤
│                    INPUT LAYER                        │
│  InputHandler (polls once → InputFrame snapshot,     │
│  emits events/callbacks; no UI dependencies)         │
├──────────────────────────────────────────────────────┤
│                DOCUMENT MANAGER                       │
│  DocumentManager                                     │
│  (infinite scroll, chapter lifecycle, anchors)       │
├──────────────────────────────────────────────────────┤
│               TEXT LAYOUT ENGINE                      │
│  LayoutEngine                                        │
│  (segment-aware layout, word wrapping, caching)      │
├──────────────────────────────────────────────────────┤
│                   DATA LAYER                          │
│  ChapterProvider (interface)                         │
│    ├── USFMParser   (offline USFM files)             │
│    └── BibleClient  (online HTML API)                │
│  PersistenceManager (SQLite)                         │
├──────────────────────────────────────────────────────┤
│                CROSS-CUTTING                          │
│  core/          Platform, Config, Theme, EnvLoader   │
│  event/         EventBus (pub-sub)                   │
│  highlight/     Highlighter (portable SimpleColor)   │
└──────────────────────────────────────────────────────┘
```

## Module Dependency Graph

```
main.cpp
  ├── App
  │     ├── Renderer         → Config, HighlightRect (SimpleColor)
  │     │     └── UIManager  → Config
  │     ├── InputHandler     → EventBus, callbacks (no UI deps)
  │     ├── NavigationStack  → Screen base
  │     │     ├── ReaderScreen   → DocMgr, Highlighter, Renderer
  │     │     ├── BookListScreen → UIScale, TapDetector
  │     │     ├── ChapterGridScreen → UIScale, TapDetector
  │     │     ├── SettingsScreen   → UIScale, TapDetector
  │     │     ├── HighlightBrowserScreen → UIScale, TapDetector
  │     │     ├── CenterMenu     → UIScale, TapDetector
  │     │     ├── CreditsOverlay → UIScale, TapDetector
  │     │     └── FontDiagnostic → (none)
  │     └── DocumentManager
  │           ├── LayoutEngine
  │           │     └── ChapterProvider (for Segment, Word, ChapterData)
  │           └── ChapterProvider (interface)
  │                 ├── USFMParser
  │                 └── BibleClient → Config
  │
  ├── Highlighter
  │     └── PersistenceInterface → PersistenceManager → Config
  │
  └── EventBus (pub-sub, no business logic)
```

All dependencies flow downward. No module depends on a higher layer.

## Module Responsibilities

| Module | Directory | Responsibility |
|--------|-----------|----------------|
| App | `app/` | Wires subsystems, owns lifecycle, delegates to NavigationStack and InputHandler |
| Config | `core/` | Constants: window size, asset paths, source selection |
| Platform | `core/` | Platform abstraction: init, I/O, clipboard, directory creation, DPI |
| EnvLoader | `core/` | `.env` file parser (for API key) |
| USFMParser | `data/` | USFM file → ChapterData (Segment[] + Word[]) |
| BibleClient | `data/` | HTML API → ChapterData (Segment[] + Word[]) |
| ChapterProvider | `data/` | Abstract interface for both data sources |
| LayoutEngine | `text/` | Segment-aware layout, word wrapping, span generation |
| DocumentManager | `document/` | Infinite scroll, chapter lifecycle (talks to ChapterProvider) |
| Highlighter | `highlight/` | Per-word highlight management (portable SimpleColor, no raylib) |
| PersistenceManager | `persistence/` | SQLite operations |
| InputHandler | `input/` | Raw input → EventBus events (no UI dependencies) |
| Renderer | `renderer/` | Top-level render coordinator, HighlightRect conversion |
| UIManager | `renderer/` | Top bar, settings, context menu, dialogs |
| RadialMenu | `renderer/` | Sector-based highlight action menu |
| Screen | `ui/` | Abstract base for on-screen views |
| NavigationStack | `ui/` | Push/pop screen management, delegates input to active screen |
| ReaderScreen | `ui/` | Main reading view (draws text + highlights) |
| BookListScreen | `ui/` | Book selection list |
| ChapterGridScreen | `ui/` | Chapter number grid |
| SettingsScreen | `ui/` | Settings panel |
| HighlightBrowserScreen | `ui/` | Highlight browsing |
| CenterMenu | `ui/` | Navigation menu overlay |
| CreditsOverlay | `ui/` | About/credits overlay |
| FontDiagnostic | `ui/` | Debug font inspector |
| TapDetector | `ui/` | Press/drag/release state helper |
| EventBus | `event/` | Pub-sub message bus (no business logic) |

## Event Governance

The EventBus provides typed pub-sub communication. All events are defined in `src/event/Events.h`.

| Event | Emitter(s) | Subscriber(s) |
|-------|-----------|---------------|
| `ScrollEvent` | InputHandler | ReaderScreen, ChapterGridScr, HighlightBrowserScr, BookListScr, DocumentManager |
| `SelectionEvent` | App (input callbacks) | Highlighter |
| `ResizeEvent` | InputHandler | App, LayoutEngine, DocumentManager |
| `FontSizeEvent` | InputHandler, SettingsScreen | App, DocumentManager |
| `SourceSwitchEvent` | SettingsScreen | App, DocumentManager |
| `DialogEvent` | InputHandler | InputHandler (self) |
| `NavigateEvent` | ReaderScreen, ChapterGridScr | App |
| `NavigateToHighlightEvent` | HighlightBrowserScr | ReaderScreen |
| `ChapterLoadedEvent` | DocumentManager | App |

Guidelines:
- Events carry data, not behavior. Keep structs small with value semantics.
- Emitters delegate to EventBus; they do not know who listens.
- Subscribers register in constructors; subscriptions are lifetime-bound.
- Prefer direct method calls over EventBus when the callee is known and owned by the same component.

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



