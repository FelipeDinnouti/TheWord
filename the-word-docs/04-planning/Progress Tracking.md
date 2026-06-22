# Progress Tracking

> Status: Phase 6 Complete | Last Updated: 2026-06-22

## Overall Progress

| Phase | Status | Target |
|-------|--------|--------|
| Phase A: Documentation Restructure | ✅ Complete | 2026-06 |
| Phase B: Code Restructure | ✅ Complete | 2026-06 |
| Phases 1-3: Core Features | ✅ Complete | 2026-06 |
| Phase 4: Architecture Foundation | ✅ Complete | 2026-06 |
| Phase 5: USFM Parser | ✅ Complete | 2026-06 |
| Phase 6: BibleClient (HTML API) | ✅ Complete | 2026-07 |
| Phase 7: Highlighting System | ⬜ Planned | 2026-08 |
| Phase 8: SQLite Persistence | ⬜ Planned | 2026-08 |
| Phase 9: UI Layer | ⬜ Planned | 2026-08 |
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

## Remaining Phases

- [ ] Phase 7: Highlighting System
- [ ] Phase 8: SQLite Persistence
- [ ] Phase 9: UI Layer polish
- [ ] Phase 10: Mobile/Android
