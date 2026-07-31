# Progress Tracking

> Status: v1.8.0-alpha complete | Last Updated: 2026-07-31

## Overall Progress

| Phase / Release | Status | Notes |
|-----------------|--------|-------|
| Phase A: Documentation Restructure | ✅ Complete | Doc tree created, cross-referenced |
| Phase B: Code Restructure | ✅ Complete | CMake fixed, assets moved, tests integrated |
| Phases 1-3: Core Features | ✅ Complete | Build, layout engine, document manager |
| Phase 4: Architecture Foundation | ✅ Complete | ChapterProvider, Segment model, Renderer extraction |
| Phase 5: USFM Parser | ✅ Complete | 66 books, 16 markers, 26 tests |
| Phase 6: BibleClient (HTML API) | ✅ Complete | Dual-source, CompositeProvider, 40 tests |
| Phase 7: Highlighting System | ✅ Complete | Per-word highlights, 9 tests |
| Phase 8: SQLite Persistence | ✅ Complete | Schema, preferences, 53 tests |
| Phase 9: UI Layer | ✅ Complete | InputHandler, UIManager, dialogs, polish, 64 tests |
| Phase 10: Mobile/Android | 📋 Backlog | Build works; remaining items deferred to Release Plan backlog |
| Phase 11: Navigation System | ✅ Complete | |
| Phase 12: Verse Number Identifiers | ✅ Complete | |
| Phase 13: Highlight Browser | ✅ Complete | |
| v1.5.0-alpha.x (UI Polish) | ✅ Complete | Final release: v1.5.0-alpha.2 (2026-07-05). No stable 1.5.0. |
| v1.6.1-alpha (Input Refactor) | ✅ Complete | Unified FSM, semantic callbacks, TapDetector, bug fixes |
| v1.6.2-alpha (Radial Menu + Lifecycle) | ✅ Complete | Sector-based hit detection, Android lifecycle fix, highlight recolor fix |
| v1.6.3-alpha (VSYNC + Idle Drain) | ✅ Complete | Android VSYNC fix, time-based idle drain, uncapped FPS on high-refresh |
| v1.7.0-alpha (Reading Experience) | ✅ Complete | Copy Verse, Footnotes, Open Where You Left Off, Immersive Mode |
| v1.8.0-alpha (Customization & Navigation) | ✅ Complete | Code Quality Audit, Fuzzy Finder, Bible Version Switcher, Modular Theme System |
| v1.9.0-alpha (Architecture Refactor + Code Quality Assurance) | 🔲 Planned | A2, A4, A5, A9 + fresh audit — current release |
| v1.10.0-alpha (Animations & UI Polish + Web Deployment) | 🔲 Planned | WASM verification, `serve-web.sh` static server, animation pass |

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
- [x] main.cpp is thin orchestrator (~250 lines)
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
- [x] Highlight rendering in Renderer via HighlightRect
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

## Phase 9 — UI Layer ✅

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
- [x] **Bug 1** — Stale context menu when G/S opens go-to/settings: `hideContextMenu()` now called before toggling dialogs
- [x] **Bug 2** — Outside-click on context menu creates spurious highlight: early `return` after `handleContextMenuClick()` prevents FSM from processing same click
- [x] **Bug 3** — `dismissActiveDialog()` dead code: unified Escape handler in `InputHandler.cpp`
- [x] **Bug 5** — Highlight orphaning on version switch: `provider_name` column added to `highlights` DB table; `Highlighter` filters by current provider; new highlights tagged with provider name; schema migration via ALTER TABLE
- [x] **Labels**: `"Src:"` → `"Source:"`, `"Clr:"` → `"Color:"` in settings panel
- [x] **Close buttons**: "X" buttons added to go-to dialog and settings panel (top-right corner)
- [x] **Font limit feedback**: A–/A+ buttons visually gray out at min(12)/max(36)
- [x] **Settings gap**: Color row offset adjusts dynamically when Source row is hidden (no API key)
- [x] **Context menu overflow**: Menu flips to left of cursor when it would overhang right edge
- [x] **FPS counter**: Gated behind `#ifndef NDEBUG` (hidden in release builds)
- [x] 64/64 tests still passing

### Sprint 4: Polish ✅
- [x] Heading differentiation — BookTitle: headingFont 1.6× BLACK centered, ChapterLabel: headingFont 1.6× gray(80,80,80) centered, SectionHeading: headingFont 1.3× DARKGRAY (as-is)
- [x] Splash screen — text-only "TheWord" at 48pt + "Loading..." at 20pt drawn before font loading
- [x] About/credits overlay — toggle with 'A', shows app name, Raylib credit, data sources, keyboard shortcuts
- [x] Error feedback for invalid go-to input (D1) — input box border turns RED on invalid Enter, clears on next keystroke
- [x] Scrollbar track visibility (D2) — widened from 4px to 6px, darkened from LIGHTGRAY to GRAY
- [x] Theme consolidation — created `src/core/Theme.h` with 27 named color constants and 4 font scale constants; all 54 inline color literals and 19 inline font scales replaced with `theme::*` references
- [x] Layout constants — settings panel grid positions, font/source button dimensions, go-to dialog spacing all extracted to named constants
- [x] Config font ranges — `FONT_SIZE_MIN` (12), `FONT_SIZE_MAX` (36), `FONT_SIZE_STEP` (2) added to Config.h; all inline min/max/step references replaced
- [x] Position fixes — about overlay text aligned to `SETTINGS_LABEL_X` (+15→+10); color swatches start at `COLOR_SWATCH_START` (+30→+60, clear of "Color:" label)
- [x] 64/64 tests still passing

## Phase 10 — Mobile/Android 📋 Backlog

### Completed
- [x] Android NDK build — `build-android/libtheword.so` via Ninja
- [x] APK packaging — `scripts/build-android.sh` produces signed `theword.apk`
- [x] AndroidManifest.xml — NativeActivity entry, INTERNET permission
- [x] `AndroidClient.cpp` — HTTP via Android native APIs
- [x] `AndroidAssetProvider.cpp` — font loading via AAssetManager
- [x] Touch gesture system — added `HandleTouchScroll`, `HandleTouchPressFSM`, `HandlePinch` in InputHandler
- [x] Android lifecycle — app state checks in main loop (`GetAndroidApp()`)
- [x] Resolution independence — DPI scaling via `AConfiguration_getDensity`
- [x] Render at native display resolution — `InitWindow(0, 0, ...)` on Android
- [x] Emscripten/WASM build — `build-wasm/theword.html` works in browser
- [x] Keyboard input patch — `patches/raylib-android-keycodes.patch` translates AKEYCODE → raylib KEY

### Remaining (tracked in Release Plan.md)
- [x] Multi-ABI build: `x86_64` + `arm64-v8a` in build script (armeabi-v7a dropped)
- [ ] Lifecycle save/restore — now part of v1.6.2
- [ ] Remaining items tracked in `Release Plan.md` backlog

## Phase 11 — Navigation System ✅ Complete

- [x] Remove top bar (chapter ref moves to bottom bar)
- [x] Bottom bar overlay on Reader (appears on scroll-up, 30px threshold, hides on scroll-down)
- [x] Bottom bar buttons: ◄ (prev chapter), ► (next chapter), center (book code, opens menu)
- [x] Center menu dialog: Books, Settings, Highlights, Credits options
- [x] BookList screen (canonical order, search bar at top, scrollable list with prefix matching)
- [x] ChapterGrid screen (5-column grid, numbered from 1 to chapterCount)
- [x] Settings screen (full-screen, replaces current modal panel)
- [x] HighlightBrowser screen (placeholder — full implementation in Phase 13)
- [x] Credits overlay (migrated from About overlay, shows version info)
- [x] Navigation stack (push/pop with back button + Escape, overlay-aware for CenterMenu/Credits)
- [x] Keyboard shortcuts: ←/→ for prev/next chapter, G for center menu, S for settings, A for credits
- [x] Reusable UI component extraction (DrawHeaderBar, DrawBottomBar helpers)
- [x] Dead code cleanup: removed GoToDialog, SettingsPanel, AboutOverlay; simplified UIManager to ContextMenu only

## Phase 12 — Verse Number Identifiers ✅ Complete

- [x] Add `SegmentType::VerseNumber` to `ChapterProvider.h`
- [x] `LayoutEngine::LayoutWords()` detects verseId transitions and inserts verse number spans
- [x] Verse number spans include space after dot, accounted for in word wrapping
- [x] `Renderer::DrawSpan()` handles VerseNumber with superscript font scale + Y offset
- [x] Theme constants: `DOC_VERSE_NUMBER` color, `FONT_VERSE_NUMBER` scale
- [x] Tests: verse numbers appear at verse boundaries, correct positioning, no verse number for verse 1 of first chapter (or always show)

## Phase 13 — Highlight Browser ✅ Complete

### Steps 1-3: Data Model, Highlighter, Persistence
- [x] Extend `Highlight` struct with bookId, chapterNum, verseStart, verseEnd, verseText
- [x] Add `NavigateToHighlightEvent` to `src/event/Events.h`
- [x] Update `Highlighter::EndSelection()` to populate reference fields from `SetChapterContext()`
- [x] Add `Highlighter::GetHighlightsByType(int typeId)` filter method
- [x] Schema migration in PersistenceManager (5 ALTER TABLE statements)
- [x] Update `LoadHighlights()` / `SaveHighlight()` for new columns
- [x] Wire `SetChapterContext()` in App.cpp on initial load and NavigateEvent
- [x] 65/65 tests passing (no regressions)

### Steps 4-7: UI & Navigation
- [x] HighlightBrowserScreen: color swatch filter at top, scrollable match list below
- [x] Match list items: reference title + verse text body
- [x] Tap match → navigate to verse in Reader (load chapter + scroll to word position)
- [x] Wire CenterMenu "Highlights" action to push HighlightBrowserScreen
- [x] Empty state: "No highlights of this color" message
- [x] Tests: filter by color, reference field population, navigation from tap

## v1.6.1-alpha — Input System Refactor + Bug Fixes ✅

- [x] Unified FSM — single `RunUnifiedFSM()` replaces dual desktop/touch FSMs
- [x] Semantic callbacks — `onTap`, `onTapEmpty`, `onDragStart`, `onDragUpdate`, `onDragEnd`, `onLongPress`, `onDismiss`
- [x] TapDetector helper — consolidated 6 screens' duplicated press→drag→release pattern
- [x] Dead event removal (`RightClickEvent`, `ScrollStopEvent`)
- [x] Dialog freeze fix — FSM resets to Idle when dialog opens
- [x] FSM guard — overlay screens blocked from word-level hits
- [x] Tap-on-release — all 6 overlay screens process taps on release, not press
- [x] Selection drag fix — active selection carries its own chapter context
- [x] Radial menu hitbox — 1.8× visual scale
- [x] Build scripts — Linux, Windows cross-compile, Android APK

## v1.6.2-alpha — Radial Menu UX/UI + Android Lifecycle ✅

- [x] Sector-based radial hit detection (replaces circular hitboxes, eliminates dead zones)
- [x] Highlight recolor fix — `HighlightOverlapping()` prevents duplicate creation
- [x] Android lifecycle — stop quitting on surface loss (`DestroyRequested()`)
- [x] Skip draw when window null, keep PollInputEvents running during surface gap
- [x] Reset input FSM on `APP_CMD_RESUME` (`InputHandler::ResetState()`)
- [x] Save/restore scroll position across OS kills
- [x] Debug tap overlay (red dot, fades over 1s)
- [x] Tagged as `v1.6.2-alpha`

## v1.6.3-alpha — Android VSYNC Fix + Idle Drain ✅

- [x] Add EGL logging after `eglSwapInterval` call (display handle, return value, error code)
- [x] Extend raylib patch — uncomment `eglSwapInterval` in `InitGraphicsDevice()`
- [x] Add `eglSwapInterval` after `eglMakeCurrent` in context rebind path
- [x] Time-based idle drain — cap draw rate at ~5fps when nothing changes
- [x] Build, deploy, logcat-verified VSYNC working on device
- [x] Tagged as `v1.6.3`

## v1.7.0-alpha — Reading Experience ✅

- [x] Copy Verse Polish — citation format, Ctrl+C, toast feedback
- [x] Footnote Display — data layer, USFM/HTML parsing, markers, popup, cross-refs
- [x] Open Where You Left Off — scroll save/restore
- [x] Immersive / Clean Mode — hide verse numbers, section headings
- [x] Verse Flow Fix — no forced line breaks between verses
- [x] Bible ID Fix (129→3034)
- [x] Tagged as `v1.7.0-alpha`

## v1.8.0-alpha — Customization & Navigation ✅

- [x] Code Quality Audit — all 8 steps (`-Wall -Wextra -Wpedantic` clean, dead code removed, includes pruned, const-correctness, error-handling, naming consistency)
- [x] Architectural Remediation — A1, A3, A6, A7, A8
- [x] Fuzzy Finder — FuzzyMatcher scoring replaces prefix-match in BookListScreen
- [x] Bible Version Switcher — curated list (BSB, WEB, ASV, BLV), `bible_id` persistence
- [x] Modular Theme System — ThemeManager + Light/Dark palettes, `dark_mode` preference
- [x] Layout cache invalidation on Bible version switch
- [x] Tagged as `v1.8.0-alpha`

## v1.9.0-alpha — Architecture Refactor + Code Quality Assurance 🔲 Planned

- [ ] A2: LayoutTypes extraction · A4: DrawContext · A5: TextMeasureFn · A9: FontManager + selection flow
- [ ] Code Quality Assurance — fresh full audit across all src/ (incl. deferred input consolidation)
- See `memory/Active.md` for the implementation checklist

## v1.10.0-alpha — Animations & UI Polish + Web Deployment 🔲 Planned

- [ ] WASM browser verification (web version never tested) + `scripts/serve-web.sh` static server
- [ ] Animations & UI Polish — animation/transition pass across screens and overlays

## Post-MVP: Release-Based Planning

All future work is organized by release (SemVer) rather than phases.
See `Release Plan.md` for current and upcoming releases.

**Remaining MVP-phase items** (Phase 10 Mobile/Android polish) are tracked
in the release plan and will be picked up as capacity allows.
