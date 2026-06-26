# Roadmap

> Status: Updated 2026-06-26 (Phase 9 complete, Phase 10 in progress)

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

## Phase 9 — MVP Final UI ✅ Complete

All 4 sprints (Foundation, Highlight UX, Navigation & Settings, Polish) completed.
- 64/64 tests passing
- InputHandler, UIManager, Renderer extracted from main.cpp
- Context menu, go-to dialog, settings panel, about overlay
- Heading differentiation, splash screen, theme consolidation
- See `04-planning/Progress Tracking.md` for full Sprint breakdown

## Phase 10 — Mobile 🔄 In Progress

| Task | Status | Notes |
|------|--------|-------|
| Phase 10a: WebAssembly build (Emscripten) | ✅ Done | `build-wasm/` produces .html + .wasm |
| Phase 10b: Touch gesture system | ✅ Done | `InputHandler` has touch scroll, press FSM, pinch |
| Phase 10c: Android NDK build | ✅ Done | `build-android/` + `scripts/build-android.sh` |
| Phase 10d: Multi-ABI (arm64-v8a) | ⬜ | Script hardcoded to x86_64 |
| Phase 10e: WASM persistence (IDBFS) | ⬜ | DB path, shell mount |
| Phase 10f: Soft keyboard (IME) | ⬜ | JNI for go-to dialog |
| Phase 10g: Lifecycle save/restore | ⬜ | Scroll position on pause |
| Phase 10h: libcurl optional | ⬜ | `QUIET` find_package |

## Dependency Notes

- **Phase 7 + 8 must be designed together**: The `Highlight` data model must match the SQLite schema. Use `PersistenceInterface` to decouple implementation order — Phase 7 can work with an in-memory stub, Phase 8 swaps in SQLite without code changes to highlight logic.
- **Phase 6 is always compiled**: Both online (BibleClient) and offline (USFMParser) providers are present in every build. At runtime, if no `.env` / API key is found, BibleClient returns `nullopt` and CompositeProvider falls back to USFMParser transparently. If YouVersion changes their HTML format, BibleClient will need updating regardless.
- **Phase 9 depends on Phase 7**: Input events (mouse click/drag) feed into highlight hit detection. Basic scroll input already works in main.cpp.
- **Phase 10 depends on Phase 9**: Touch gestures replace mouse/keyboard input. Recommended intermediate step: WebAssembly via Emscripten before Android.
