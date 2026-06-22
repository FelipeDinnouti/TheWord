# Roadmap

> Status: Updated 2026-06-22

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

## Short Term (July 2026)

## Immediate (July 2026)

| Task | Priority | Est. Effort |
|------|----------|-------------|
| Phase 7a: Highlighter + PersistenceInterface | 🔴 High | 2d |
| Phase 7b: Hit detection | 🔴 High | 1.5d |
| Phase 7c: Drag selection | 🔴 High | 1.5d |
| Phase 7d: Highlight rectangle rendering in Renderer | 🔴 High | 1d |
| Phase 7e: Input event wiring | 🟡 Medium | 1d |
| Phase 8a: SQLite PersistenceManager | 🟡 Medium | 1d |
| Phase 8b: Replace in-memory stub, integrate | 🟡 Medium | 1d |
| Phase 8c: Persistence unit tests | 🟡 Medium | 1d |

## Medium Term (August 2026)

| Task | Priority | Est. Effort |
|------|----------|-------------|
| Phase 9a: InputHandler class (extract from main.cpp) | 🟡 Medium | 1d |
| Phase 9b: UIManager (book/chapter navigation UI) | 🟡 Medium | 2d |
| Phase 9c: Font size controls | 🟢 Low | 1d |
| Phase 9d: Smooth scroll refinements | 🟢 Low | 1d |
| Phase 9e: Window resize polish (basic resize already in main.cpp) | 🟢 Low | 0.5d |
| Phase 9f: Heading differentiation (BookTitle/SectionHeading/ChapterLabel styles) | 🟢 Low | 0.5d |

## Long Term (September+)

| Task | Priority | Est. Effort |
|------|----------|-------------|
| Phase 10a: WebAssembly build (Emscripten) | 🟢 Low | 2d |
| Phase 10b: Touch gesture system | 🟢 Low | 2d |
| Phase 10c: Android NDK + lifecycle | 🟢 Low | 3d |
| Multiple highlight colors | 🟢 Low | 2d |
| Bible version switching | 🟢 Low | 3d |

## Dependency Notes

- **Phase 7 + 8 must be designed together**: The `Highlight` data model must match the SQLite schema. Use `PersistenceInterface` to decouple implementation order — Phase 7 can work with an in-memory stub, Phase 8 swaps in SQLite without code changes to highlight logic.
- **Phase 6 is always compiled**: Both online (BibleClient) and offline (USFMParser) providers are present in every build. At runtime, if no `.env` / API key is found, BibleClient returns `nullopt` and CompositeProvider falls back to USFMParser transparently. If YouVersion changes their HTML format, BibleClient will need updating regardless.
- **Phase 9 depends on Phase 7**: Input events (mouse click/drag) feed into highlight hit detection. Basic scroll input already works in main.cpp.
- **Phase 10 depends on Phase 9**: Touch gestures replace mouse/keyboard input. Recommended intermediate step: WebAssembly via Emscripten before Android.
