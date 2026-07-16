# Release Plan

> Status: v1.6.x complete | Last Updated: 2026-07-14

## Past Releases

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

## Current Release: v1.7.0-alpha

**Theme:** Reading Experience — Copy Verse, Footnotes, Open Where You Left Off, Immersive Mode

### Feature: Copy Verse Polish

- [x] Citation format: `Book Chapter:Verse\n\n<text>` with two line breaks
- [x] Ctrl+C keyboard shortcut on desktop
- [x] Toast/popup feedback ("Copied!") after copy
- [x] Refactor `AssembleSelectedText()` for citation prefix

### Feature: Footnote Display

- [x] Data layer — `Footnote` struct, `SegmentType::FootnoteMarker`, `ChapterData::footnotes`
- [x] USFM parser — extract footnotes instead of stripping (`\f ... \f*`)
- [x] BibleClient — extract footnotes from HTML instead of stripping (`<span class="yv-n">`)
- [x] LayoutEngine — insert `[n]` superscript marker spans
- [x] Renderer — draw markers with superscript style
- [x] Interaction — tap marker → popup, tap-away → dismiss (integrate with UnifiedFSM)
- [x] FootnotePopup component (UIManager)
- [x] Cross-reference (`\rq`) promotion to footnotes

### Feature: Open Where You Left Off

- [x] Save `last_scroll` on app pause/close (desktop + Android)
- [x] Restore scroll position after chapter load on startup
- [x] Reuse/extend existing `lifecycle_scroll` pattern

### Feature: Immersive / Clean Mode

- [x] Hide verse numbers, chapter labels, section headings
- [x] Toggle via 'I' hotkey + SettingsScreen switch
- [x] Persist setting in preferences

### Release Steps

- [x] Update `State.md`
- [x] Bump version in `CMakeLists.txt` (1.6.3 → 1.7.0), reconfigure, build
- [x] Full test suite passes (75/77)
- [x] Manual verification on Android
- [ ] Tag release `v1.7.0-alpha`

## Backlog (Future Releases)

### Phase 10 Remaining Items (deferred)

- [x] Java activity stub (`TheWordActivity.java`) for IME/splash — done
- [x] Soft keyboard integration for go-to dialog — done
- [ ] Complex IME composition (CJK, emoji) — would require hidden EditText + TextWatcher (future)
- [ ] Android immersive mode (hide system nav bar) — distinct from in-app Clean Mode
- [x] Lifecycle save/restore (scroll position on pause/resume) — done in v1.6.2-alpha
- [ ] WASM persistence via IDBFS
- [ ] `scripts/build-wasm.sh` for one-step WASM build
- [ ] libcurl as optional dependency
- [ ] Platform abstraction module (`src/core/Platform.h`)

### Feature Backlog

- Note System (details TBD)
- Custom Reading Plan System (details TBD)
- Bookmark System (expansion of highlight system, details TBD)
- **Bible Version Switcher** — dropdown/selector in SettingsScreen to choose from enabled YouVersion IDs; persist choice; re-init `BibleClient` + `CompositeProvider` on change
- Search across books/chapters
- Dark / sepia theme
- Night mode
- Reading progress tracking
- Font selection (system fonts)
- Highlight export/import
- Performance profiling & optimization
- Non-contiguous verse selection
- Code quality audit beyond input system
