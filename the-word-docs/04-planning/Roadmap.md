# Roadmap

> Status: Updated 2026-06-22 (Phase 9 plan finalized)

## ✅ Completed

### Phase A: Documentation Restructure
- All docs written, cross-referenced, and organized under `the-word-docs/`

### Phase B: Code Restructure
- Directory structure cleaned, CMake fixed, assets moved, doctest integrated

### Phases 1-3: Core Features
- Build system, window, core utilities, BibleBooks, LayoutEngine, DocumentManager

### Phase 4: Architecture Foundation
- Renderer extracted from main.cpp, ChapterProvider/ChapterData/Segment types defined, LayoutEngine segment-aware, DocumentManager refactored, StubChapterProvider, segment-aware rendering

### Phase 5: USFM Parser
- USFMParser implements ChapterProvider (66 books, 16 supported markers, footnote stripping, book-level caching), 26/26 tests passing

### Phase 6: BibleClient (HTML API)
- BibleClient wired into main.cpp via CompositeProvider (runtime fallback)
- Both providers always compiled (libcurl hard dependency)
- 14 new unit tests: 8 HTML parsing + 6 CompositeProvider
- 40/40 tests passing

### Phase 7: Highlighting System
- Highlighter class with PersistenceInterface
- Hit detection (screen → word), drag selection
- Highlight rectangle rendering in Renderer
- 9 unit tests (7 Highlighter + 2 InMemoryStorage)

### Phase 8: SQLite Persistence
- PersistenceManager implements PersistenceInterface
- SQLite schema: highlight_types, highlights, preferences
- 4 unit tests (save/load, remove, types, preferences)
- 53/53 tests passing

## Phase 9 — MVP Final UI (July 2026)

### Sprint 1: Foundation

| Task | Priority | Est. Effort |
|------|----------|-------------|
| 9a: InputHandler class (extract from main.cpp) | 🟡 Medium | 1d |
| 9b: UIManager (top bar, settings, dialogs) | 🟡 Medium | 2d |
| 9c: Smooth scroll refinements | 🟢 Low | 1d |
| 9d: Window resize polish | 🟢 Low | 0.5d |

### Sprint 2: Highlight UX

| Task | Priority | Est. Effort |
|------|----------|-------------|
| 9e: Highlight hit-testing (`highlightAtWord`) | 🔴 High | 0.5d |
| 9f: Context menu (trash + 5 pastel color swatches) | 🔴 High | 1.5d |
| 9g: Multiple highlight colors + seeding | 🟡 Medium | 1d |
| 9h: Delete highlight flow | 🔴 High | 0.5d |

### Sprint 3: Navigation & Settings

| Task | Priority | Est. Effort |
|------|----------|-------------|
| 9i: Book/chapter navigation dialog | 🟡 Medium | 2d |
| 9j: Font size controls (live update + persist) | 🟢 Low | 1d |
| 9k: Bible version switching (settings toggle) | 🟡 Medium | 1d |
| 9l: Preferences persistence (font, version, color) | 🟢 Low | 0.5d |

### Sprint 4: Polish

| Task | Priority | Est. Effort |
|------|----------|-------------|
| 9m: Heading differentiation (3 styles) | 🟢 Low | 0.5d |
| 9n: Splash screen (text-only) | 🟢 Low | 0.5d |
| 9o: About/credits overlay | 🟢 Low | 0.5d |

## Phase 10 — Mobile (September+)

| Task | Priority | Est. Effort |
|------|----------|-------------|
| Phase 10a: WebAssembly build (Emscripten) | 🟢 Low | 2d |
| Phase 10b: Touch gesture system | 🟢 Low | 2d |
| Phase 10c: Android NDK + lifecycle | 🟢 Low | 3d |

## Dependency Notes

- **Phase 7 + 8 must be designed together**: The `Highlight` data model must match the SQLite schema. Use `PersistenceInterface` to decouple implementation order — Phase 7 can work with an in-memory stub, Phase 8 swaps in SQLite without code changes to highlight logic.
- **Phase 6 is always compiled**: Both online (BibleClient) and offline (USFMParser) providers are present in every build. At runtime, if no `.env` / API key is found, BibleClient returns `nullopt` and CompositeProvider falls back to USFMParser transparently. If YouVersion changes their HTML format, BibleClient will need updating regardless.
- **Phase 9 depends on Phase 7**: Input events (mouse click/drag) feed into highlight hit detection. Basic scroll input already works in main.cpp.
- **Phase 10 depends on Phase 9**: Touch gestures replace mouse/keyboard input. Recommended intermediate step: WebAssembly via Emscripten before Android.
