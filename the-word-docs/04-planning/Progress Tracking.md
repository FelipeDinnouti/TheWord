# Progress Tracking

> Status: Phase 8 Complete | Last Updated: 2026-06-22

## Overall Progress

| Phase | Status | Target |
|-------|--------|--------|
| Phase A: Documentation Restructure | ✅ Complete | 2026-06 |
| Phase B: Code Restructure | ✅ Complete | 2026-06 |
| Phases 1-3: Core Features | ✅ Complete | 2026-06 |
| Phase 4: Architecture Foundation | ✅ Complete | 2026-06 |
| Phase 5: USFM Parser | ✅ Complete | 2026-06 |
| Phase 6: BibleClient (HTML API) | ✅ Complete | 2026-07 |
| Phase 7: Highlighting System | ✅ Complete | 2026-08 |
| Phase 8: SQLite Persistence | ✅ Complete | 2026-08 |
| Phase 9: UI Layer | 🏃 In Progress | 2026-08 |
| Phase 10: Mobile Preparation | ⬜ Planned | 2026-09 |

## Phase A — Documentation Restructure ✅

- [x] Persist restructuring plan (`.opencode/plan.md`)
- [x] Create new doc directory structure
- [x] Write 00-INDEX.md
- [x] Write vision docs (Project Specification, Product Requirements)
- [x] Write architecture docs (Overview, Data Structures, Coordinates, Data Flow)
- [x] Write module docs (Core, Bible API, Layout Engine, Document Manager)
- [x] Write planned module specs (USFM, Highlighting, Persistence, UI)
- [x] Write planning docs (Development Plan, Progress Tracking, Roadmap)
- [x] Write reference docs (USFM Format, YouVersion API, Raylib Notes)
- [x] Write ops docs (Build Guide, Environment Setup, Troubleshooting)
- [x] Write AI collaboration docs (Agent Workflow, Doc-First Checklist, Conventions)
- [x] Simplify AGENTS.md
- [x] Simplify README.md
- [x] Clean up old the-word-docs files

## Phase B — Code Restructure ✅

- [x] Remove `include/` and `libs/` directories (FetchContent provides headers)
- [x] Move data files (JSON, fonts) to `assets/fonts/` and `assets/json/`
- [x] Fix CMakeLists.txt (explicit file lists, no GLOB_RECURSE)
- [x] Add `tests/` directory with doctest (5/5 tests passing)
- [x] Add `scripts/` directory
- [x] Remove empty `data/` directory
- [x] Verify `./build/theword` runs with new asset paths

## Phases 1-3 — Core Features ✅

- [x] Build system, window, core utilities
- [x] Text layout engine (tokenization, word wrapping, caching)
- [x] BibleBooks.h — data-driven 66-book table
- [x] Document Manager rewrite — clean interface, anchor-fixed prepend/append
- [x] Chapter navigation across all 66 books
- [x] Decoupled getVisibleSpans
- [x] 18/18 tests passing
- [x] App builds and runs

## Phase 4 — Architecture Foundation ✅

- [x] Data model unification — single source of truth for Word/Span/Line/ChapterLayout in ChapterProvider.h
- [x] SegmentType field added to Span for renderer-aware drawing
- [x] Config.h cleaned up — API constants removed, font paths added
- [x] LayoutEngine refactored — accepts ChapterData, segment-aware (VerseText, SectionHeading, PoetryLine, ParagraphBreak)
- [x] DocumentManager refactored — holds ChapterProvider&, auto-loads chapters via tryPrepend/tryAppend
- [x] StubChapterProvider created for offline development
- [x] BibleClient rewritten — implements ChapterProvider, HTML format, segment parsing
- [x] Renderer extracted from main.cpp — drawFrame, scrollbar, span rendering
- [x] Segment-aware rendering — headings centered/bold, poetry indented
- [x] main.cpp is thin orchestrator (~60 lines)
- [x] Removed unused BibleVerse.h

## Phase 5 — USFM Parser ✅

- [x] Download Bíblia Livre USFM files (66 books, CC BY 4.0) → `assets/usfm/`
- [x] Rename files to `{BOOK_CODE}.usfm` format
- [x] Create `USFMParser.h/cpp` implementing `ChapterProvider`
- [x] Supported markers: `\id`, `\h`, `\c`, `\v`, `\p`, `\m`, `\s1`-`\s5`, `\q`/`\q1`-`\q3`, `\mt1`-`\mt4`, `\d`, `\r`
- [x] Footnote stripping (`\f ... \f*`)
- [x] Inline marker stripping (`\add ... \add*`, `\wj ... \wj*`)
- [x] Book-level caching (parsed once, cached for subsequent requests)
- [x] ChapterLabel, BookTitle, SectionHeading, ParagraphBreak, VerseText, PoetryLine segment types supported
- [x] Wire USFMParser in `main.cpp` — app starts at Genesis 1
- [x] Unit tests: Genesis 1 structure, HasChapter, book caching, missing file handling
- [x] 26/26 tests passing
- [x] `assets/usfm/.gitkeep` removed (now has real files)

## Phase 6 — BibleClient (HTML API) ✅

- [x] Wire BibleClient into main.cpp via runtime API key check
- [x] Create CompositeProvider for transparent fallback (online → USFM)
- [x] Dual-source always compiled (libcurl hard dependency)
- [x] BibleClient HTML parsing unit tests (8 tests: headings, paragraphs, poetry, footnotes, entities)
- [x] CompositeProvider unit tests (6 tests: primary success, fallback, both fail, HasChapter)
- [x] 40/40 tests passing

## Phase 7 — Highlighting System ✅

- [x] PersistenceInterface abstract base class defined
- [x] InMemoryStorage implements PersistenceInterface for lightweight testing
- [x] Highlighter class — startSelection, updateSelection, endSelection with word-to-word range
- [x] Highlight highlighting in renderer via HighlightRect
- [x] 7 Highlighter unit tests + 2 InMemoryStorage tests
- [x] Highlights persist within session (in-memory)

## Phase 8 — SQLite Persistence ✅

- [x] PersistenceManager implements PersistenceInterface — same contract as InMemoryStorage
- [x] SQLite amalgamation via FetchContent (no system dependency)
- [x] sqlite3.c compiled as OBJECT library, linked to both theword and theword_test
- [x] DB schema: highlight_types, highlights, preferences tables
- [x] Yellow highlight type seeded on first run
- [x] ~/.theword/ directory auto-created; :memory: support for tests
- [x] 4 PersistenceManager tests (save/load, remove, types, preferences)
- [x] 53/53 tests passing
- [x] Highlights survive app restart (SQLite-backed)

## Phase 9 — UI Layer Polish

### Sprint 1: Foundation ✅

- [x] InputHandler class — extracted mouse/keyboard/wheel/highlight/resize from main.cpp into `src/input/InputHandler.h/cpp`
- [x] UIManager class — created `src/renderer/UIManager.h/cpp` with top bar, settings/context menu stubs
- [x] Smooth scroll refinements — frame-rate-independent exponential ease (`1 - exp(-k * dt)`)
- [x] Window resize polish — scroll-fraction anchor to keep content stable on resize
- [x] 53/53 tests still passing

### Sprint 2: Highlight UX ✅

- [x] Highlight hit-testing (`Highlighter::highlightAtWord`) — returns `const Highlight*` for context menu
- [x] Context menu widget — side-by-side "Del" button + 5 pastel color swatches, triggered by long-press (500ms) or right-click
- [x] Multiple highlight colors — 5 pastel types seeded in DB (Yellow, Pink, Green, Blue, Orange), `Highlighter::activeTypeId` controls new highlight color
- [x] Delete highlight — "Del" in context menu → `Highlighter::removeHighlight(id)` → `persistence.removeHighlight(id)`
- [x] Recolor highlight — swatch click → `Highlighter::recolorHighlight(id, typeId)` → `persistence.saveHighlight()` (INSERT OR REPLACE)
- [x] Long-press FSM in InputHandler (Idle→Pending→Dragging/LongPress) coexists with drag selection; right-click immediate trigger
- [x] Escape dismisses context menu
- [x] 62/62 tests passing (+9 new tests)

### Sprint 3: Navigation & Settings ✅

- [x] Book/chapter navigation dialog — go-to dialog with text input, auto-complete (book code/full name prefix match, up to 5 suggestions), keyboard handling (Enter navigates, Tab auto-completes, Up/Down cycle, Backspace/Escape)
- [x] Font size controls — A–/A+ buttons (12–36 range, clamped), live update via layoutEngine.setFontSize + invalidateCache + renderer.setFontSize + docManager.invalidateLayouts, persisted
- [x] Bible version switching — settings toggle via CompositeProvider::setPrimary with USFM/Online buttons, reloads current chapter after switch, persisted
- [x] Color swatch selector in settings — black border highlight on active color, persisted
- [x] Keyboard shortcuts — 'G' toggles go-to dialog, 'S' toggles settings, Escape dismisses any active dialog
- [x] Dialog routing in InputHandler — when go-to or settings active, suppresses normal input and routes keyboard/mouse to dialog handlers
- [x] Preference loading on startup — font_size, active_version, active_color loaded from DB and applied before initial chapter load
- [x] Dual provider creation in main.cpp — always creates both USFMParser and BibleClient+CompositeProvider (when API key present); no-API-key fallback uses USFMParser for both
- [x] CompositeProvider setPrimary tests (2 new tests)
- [x] 64/64 tests passing

### Sprint 3.5: Bug Fixes & UX Polish ✅

- [x] **Bug 1** — Stale context menu when G/S opens go-to/settings: `hideContextMenu()` now called before toggling dialogs (`InputHandler.cpp:47-54`)
- [x] **Bug 2** — Outside-click on context menu creates spurious highlight: early `return` after `handleContextMenuClick()` prevents FSM from processing same click (`InputHandler.cpp:41-45`)
- [x] **Bug 3** — `dismissActiveDialog()` dead code: unified Escape handler in `InputHandler.cpp` now calls `dismissActiveDialog()` for all dialogs
- [x] **Bug 5** — Highlight orphaning on version switch: `provider_name` column added to `highlights` DB table; `Highlighter` filters by current provider; new highlights tagged with provider name; schema migration via ALTER TABLE
- [x] **Labels**: `"Src:"` → `"Source:"`, `"Clr:"` → `"Color:"` in settings panel
- [x] **Close buttons**: "X" buttons added to go-to dialog and settings panel (top-right corner)
- [x] **Font limit feedback**: A–/A+ buttons visually gray out at min(12)/max(36)
- [x] **Settings gap**: Color row offset adjusts dynamically when Source row is hidden (no API key)
- [x] **Context menu overflow**: Menu flips to left of cursor when it would overhang right edge
- [x] **FPS counter**: Gated behind `#ifndef NDEBUG` (hidden in release builds)
- [x] 64/64 tests still passing

### Sprint 4: Polish ⬜

- [ ] Heading differentiation — split BookTitle, SectionHeading, ChapterLabel into distinct render styles
  - BookTitle: headingFont at 1.6×, BLACK, centered
  - SectionHeading: headingFont at 1.3×, DARKGRAY, centered (as-is)
  - ChapterLabel: headingFont at 1.6×, gray (80,80,80), centered
- [ ] Splash screen (text-only "TheWord" + "Loading...")
- [ ] About/credits overlay

## Phase 10 — Mobile/Android ⬜

- [ ] WebAssembly build (Emscripten)
- [ ] Touch gesture system (swipe, tap, pinch)
- [ ] Android NDK build + lifecycle

## Remaining Phases (summary)

| Phase | Status |
|-------|--------|
| Phase 9: UI Layer polish | 🏃 In Progress |
| Phase 10: Mobile/Android | ⬜ Planned |
