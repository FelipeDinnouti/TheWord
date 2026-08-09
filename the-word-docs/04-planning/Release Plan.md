# Release Plan

> Status: v1.9.0-alpha released 2026-08-09 (audit pending) | Last Updated: 2026-08-09

## Past Releases

### v1.9.0-alpha (2026-08-09)

**Theme:** Architecture Refactor — findings A2, A4, A5, A9 + fresh audit

> Architecture remediation complete; APK device-verified by user (no regressions,
> no visual alteration). Code Quality Assurance audit remains (next workstream).

- [x] A2 — LayoutTypes extraction: `Span`/`Line`/`ChapterLayout` → `text/LayoutTypes.h`; no renderer→data include
- [x] A5 — TextMeasureFn abstraction: `text/` layer raylib-free, LayoutEngine testable/portable
- [x] A9.1 — selection-hop removal: InputHandler emits SelectionEvent, only Highlighter listens
- [x] A9.2-3 — FontManager extraction: App slimmed; Settings font-size bug fixed
- [x] A4 — DrawContext: ~80 direct raylib draw calls removed, `Screen::Draw(DrawContext&)`, Renderer ctor 9→1 params
- [x] Desktop preflight: 0 warnings, 78/80 tests (baseline)
- [x] Android device verification (arm64-v8a APK): theme toggle, font size, all screens/overlays, reader interactions
- [x] Tag: `git tag -am "v1.9.0-alpha" "v1.9.0-alpha"`

### v1.8.0-alpha (2026-07-20)

**Theme:** Customization & Navigation — Code Quality Audit, Fuzzy Finder, Bible Version Switcher, Modular Theme System

- [x] Code Quality Audit — `-Wall -Wextra -Wpedantic` clean, dead code removed (GlobalId.h, commented blocks), includes pruned, const-correctness pass, error-handling check, naming consistency (input consolidation deferred to v1.9 audit)
- [x] Architectural Remediation — A1, A3, A6, A7, A8 (see `memory/Architecture-Analysis.md`)
- [x] Bible Version Switcher — curated list (BSB 3034, WEB 206, ASV 12, BLV 0=USFM), single-primary CompositeProvider, `bible_id` preference persisted, `SourceSwitchEvent` removed
- [x] Modular Theme System — ThemeManager + ThemePalette (Light/Dark), `dark_mode` preference, 86 call sites across 11 files migrated
- [x] Fuzzy Finder — FuzzyMatcher scoring replaces prefix-match in BookListScreen (GoTo dialog deferred: none exists)
- [x] Layout cache invalidation on Bible version switch
- [x] Tag: `git tag -am "v1.8.0-alpha" "v1.8.0-alpha"`

### v1.7.0-alpha (2026-07-16)

**Theme:** Reading Experience — Copy Verse, Footnotes, Open Where You Left Off, Immersive Mode

- [x] Copy Verse Polish — citation format, Ctrl+C, toast feedback
- [x] Footnote Display — data layer, USFM/HTML parsing, markers, popup, cross-refs
- [x] Open Where You Left Off — scroll save/restore
- [x] Immersive / Clean Mode — hide verse numbers, section headings
- [x] Verse Flow Fix — no forced line breaks between verses
- [x] Bible ID Fix (129→3034)
- [x] Tag: `git tag -am "v1.7.0-alpha" "v1.7.0-alpha"`

### v1.6.3-alpha (2026-07-13)

**Theme:** Android VSYNC Fix + Idle Drain

- [x] EGL VSYNC logging — verify `eglSwapInterval()` actually working
- [x] Raylib patch — uncommented `eglSwapInterval` in `InitGraphicsDevice()`, added after `eglMakeCurrent` in rebind path
- [x] Time-based idle drain — cap draw rate at ~5fps when nothing changes
- [x] Build, deploy, logcat-verified VSYNC working on device
- [x] Tag: `git tag -am "v1.6.3-alpha — Android VSYNC fix, time-based idle drain, uncapped FPS on high-refresh" "v1.6.3"`

### v1.6.2-alpha (2026-07-12)

**Theme:** Sector-Based Radial Menu + Android Lifecycle Fix

- [x] Sector-based radial hit detection — eliminates dead zones between buttons
- [x] Highlight recolor fix — `HighlightOverlapping()` prevents duplicate highlights on recolor
- [x] Android lifecycle fix — stop quitting on surface loss (`DestroyRequested()`), skip draw when window null
- [x] Reset input FSM on `APP_CMD_RESUME`
- [x] Save/restore scroll position across OS kills
- [x] Debug tap overlay (red dot, fades over 1s)
- [x] Tag: `git tag -am "v1.6.2-alpha" "v1.6.2-alpha"`

### v1.6.1-alpha (2026-07-10)

**Theme:** Input System Refactor + Bug Fixes

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

### v1.5.0-alpha.2 (2026-07-05)

**Theme:** UI/UX Polish & Cross-Platform Verification (continued)

- [x] Version bump: `1.5.0-alpha.1` → `1.5.0-alpha.2`
- [x] Android keyboard integration (backspace in search bar, soft keyboard show on tap)
- [x] VSync on desktop and Android
- [x] Time-based momentum kill (Hz-independent)
- [x] Cross-compile Windows binary, switch default to NVI, remove dead shaders
- [x] UI polish pass (rounded corners, hover/press states, fade-in animations, component primitives)

### v1.5.0-alpha.1 (2026-07-04)

**Theme:** UI/UX Polish & Cross-Platform Verification

- [x] Non-blocking async chapter loading (std::async, ChapterLoadedEvent, future graveyard)
- [x] Android text input patch (GetCharPressed via JNI, gamepad flag fix)
- [x] Visual feedback: selection tint, hover/press states, cursor changes, overlay fade-in
- [x] Rounded corners on all panels, menus, buttons, grid
- [x] Bug fixes: flickering, highlight orphaning, highlight navigation, corrupted DB

### v1.4.2 (2026-06-30)

**Theme:** MVP Completion Milestone

- [x] Phase 13 Highlight Browser — data model + persistence + UI complete
- [x] Tag: `git tag -am "v1.4.2" "v1.4.2"`

## Current Release: v1.9.0-alpha

**Theme:** Architecture Refactor + Code Quality Assurance — findings A2, A4, A5, A9 + fresh audit

> Architecture remediation (A2, A4, A5, A9) **released and device-verified
> 2026-08-09**. The fresh code-quality audit below is the remaining release item.

> Full scope confirmed 2026-07-31: the Roadmap scope wins over the earlier
> Architecture-Analysis deferral — A4 and A9.2-3 are in this release.
> See `memory/Architecture-Analysis.md` for details on each finding.

### Feature: Architectural Remediation (released 2026-08-09)

- [x] A2: Extract Span/Line/ChapterLayout from `data/ChapterProvider.h` to `text/LayoutTypes.h` (Medium)
- [x] A5: Abstract `MeasureTextEx` behind TextMeasureFn interface (Medium)
- [x] A9.1: Remove selection re-emission hop through App (Small)
- [x] A9.2-3: Extract FontManager, move resize handling from App (Medium-Large)
- [x] A4: Introduce DrawContext, eliminate direct raylib calls from all screens (Large)

### Feature: Code Quality Assurance

- [ ] Fresh full audit pass across all `src/` (read + fix) — includes the input-consolidation work deferred from v1.8 (TapDetector + mouse/press boilerplate across screens)
- [ ] Granular checklist TBD during implementation planning

### Release Steps

- [x] Bump version in `CMakeLists.txt` (1.8.0 → 1.9.0-alpha), reconfigure, build
- [x] Full test suite passes (78/80 baseline)
- [x] Manual verification on Android (2026-08-09)
- [x] Tag release `v1.9.0-alpha`

## Planned: v1.10.0-alpha

**Theme:** Animations & UI Polish + Web Deployment

### Feature: Web Deployment

- [ ] Verify WASM build in a browser — build via `scripts/build-wasm.sh`, run, fix issues (web version was never tested)
- [ ] Add `scripts/serve-web.sh` — simple static server for `build-wasm/` with correct MIME types (`application/wasm`), formalizing the `python3 -m http.server` one-liner from the Build Guide
- [ ] IDBFS persistence remains backlogged — highlights are session-only on WASM in this release

### Feature: Animations & UI Polish

- [ ] Animation / transition pass across screens and overlays
- [ ] UI polish items (checklist TBD during implementation planning)

## Backlog (Future Releases)

### Phase 10 Remaining Items (deferred)

- [x] Java activity stub (`TheWordActivity.java`) for IME/splash — done
- [x] Soft keyboard integration for go-to dialog — done
- [ ] Complex IME composition (CJK, emoji) — would require hidden EditText + TextWatcher (future) - Emojis don't seem to be necessary and sadly CJK is somewhat out of scope for now.
- [ ] Android immersive mode (hide system nav bar) — distinct from in-app Clean Mode
- [x] Lifecycle save/restore (scroll position on pause/resume) — done in v1.6.2-alpha
- [ ] WASM persistence via IDBFS
- [x] `scripts/build-wasm.sh` for one-step WASM build — done
- [x] libcurl as optional dependency — done (`find_package(CURL QUIET)`, USFM-only mode without it)
- [ ] Platform abstraction module (`src/core/Platform.h`)

### Feature Backlog

- Note System (details TBD)
- Custom Reading Plan System (details TBD)
- Bookmark System (expansion of highlight system, details TBD)
- Search across books/chapters
- Sepia theme
- Reading progress tracking
- Font selection (system fonts)
- Highlight export/import
- Performance profiling & optimization
- Non-contiguous verse selection
