# Development Plan

> Status: Updated 2026-06-22

## Phase Overview

| Phase | Status | Description |
|-------|--------|-------------|
| 1. Project Foundation | ✅ Complete | Build system, directory structure, window setup |
| 2. Text Layout Engine | ✅ Complete | Tokenization, word wrapping (pre-segment) |
| 3. Document Manager | ✅ Complete | Infinite scroll, chapter lifecycle |
| 4. Architecture Foundation | ✅ Complete | Renderer extraction, ChapterProvider, Segment model |
| 5. USFM Parser | ✅ Complete | Offline Bible data source |
| 6. BibleClient (HTML API) | ✅ Complete | Online data source |
| 7. Highlighting System | ✅ Complete | Per-word selection, highlight rendering |
| 8. SQLite Persistence | ✅ Complete | Save/load highlights and preferences |
| 9. UI Layer | ⬜ Planned | Navigation, font controls, smooth scroll, heading differentiation |
| 10. Mobile Preparation | ⬜ Planned | Android build, touch input, lifecycle |

## Detailed Phase Descriptions

### Phase 1: Project Foundation ✅

**What was built:**
- CMake build system with FetchContent for Raylib
- Modular directory structure under `src/`
- Window initialization with mobile aspect ratio (450×800)
- Core utilities: Config.h, BibleBooks.h

### Phase 2: Text Layout Engine ✅

**What was built:**
- Tokenization of verse text into Word structs
- Word wrapping using `MeasureTextEx`
- Span generation with document-space coordinates
- Layout caching keyed by chapter ID
- YouVersion API integration (BibleClient, pre-HTML parser)

### Phase 3: Document Manager ✅

**What was built:**
- Data-driven book table (BibleBooks.h — 66 books)
- Infinite scroll with prepend/append
- Anchor-fixed scroll adjustment
- Chapter navigation across all books
- Decoupled `getVisibleSpans` returning document-space Y

### Phase 4: Architecture Foundation ✅

**Goal:** Clean the render loop and define the dual-source data contract.

**Status:** Complete.

- Extract Renderer class from main.cpp
- Define `ChapterProvider` interface + `ChapterData` + `Segment` types
- Refactor LayoutEngine to accept `ChapterData` (segment-aware layout)
- Refactor DocumentManager to hold `ChapterProvider&`
- Create stub ChapterProvider for development
- Implement segment-aware rendering (headings centered, poetry indented)
- Write `src/data/ChapterProvider.h`

**See:** `02-architecture/Data Source Architecture.md`

### Phase 5: USFM Parser ✅

**Goal:** App reads Bíblia Livre USFM files as offline data source.

**Status:** Complete.

- Implement USFMParser (`\c`, `\v`, `\p`, `\q1`-`\q3`, `\s1`-`\s5`, `\mt1`-`\mt4`)
- Download Bíblia Livre USFM → `assets/usfm/`
- Wire USFMParser as ChapterProvider into DocumentManager
- libcurl and EnvLoader are always compiled (hard dependency for dual-source architecture)
- Unit tests for USFM parsing

**See:** `03-modules/USFM Parser.md`

### Phase 6: BibleClient (HTML API) ✅

**Goal:** Add YouVersion API as a second ChapterProvider. Both online and offline providers are always compiled; runtime fallback selects the active source.

**Status:** Complete.

- Rewrite BibleClient to parse HTML format (`<div class="p/q1/q2/s1/s2">`)
- Strip footnotes from HTML
- Implement ChapterProvider interface
- Create CompositeProvider for transparent runtime fallback (primary→fallback)
- Wire into main.cpp — if `.env` / API key present, BibleClient is primary with USFMParser fallback; otherwise USFMParser only
- Unit tests: 8 BibleClient HTML parsing tests, 6 CompositeProvider tests

**See:** `03-modules/Bible API.md`

### Phase 7: Highlighting System ✅

**Goal:** Select words, highlight them with yellow.

**Status:** Complete.

- Highlighter class with PersistenceInterface
- Hit detection (screen → word)
- Drag selection
- Highlight rectangle rendering
- Wire input events
- 9 unit tests (7 Highlighter + 2 InMemoryStorage)

**See:** `03-modules/Highlighting System.md`

### Phase 8: SQLite Persistence ✅

**Goal:** Highlights survive app restart.

**Status:** Complete.

- PersistenceManager implementing PersistenceInterface
- SQLite schema for highlights and preferences
- Replace in-memory stub with SQLite-backed persistence
- 4 unit tests (save/load, remove, types, preferences)
- 53/53 tests passing

**See:** `03-modules/Persistence.md`

### Phase 9: UI Layer ⬜

**Goal:** Navigation, font controls, smooth scrolling polish, heading differentiation.

- InputHandler class (extract mouse/keyboard/wheel from main.cpp)
- UIManager (chapter title, font controls, navigation UI)
- Book/chapter navigation
- Smooth scroll refinements
- Window resize polish
- Heading differentiation — split BookTitle, SectionHeading, ChapterLabel into distinct render styles

**See:** `03-modules/UI Layer.md`

### Phase 10: Mobile Preparation ⬜

- WebAssembly build + test
- Touch gesture system
- Android NDK build + lifecycle
