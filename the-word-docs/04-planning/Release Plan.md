# Release Plan

> Status: v1.6.2 in development | Last Updated: 2026-07-12

## Past Releases

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

## Current Release: v1.6.2

**Theme:** Radial Menu UX/UI + Android Lifecycle

### Feature: Radial Menu UX/UI

- [ ] Larger touch targets — button radius to 28dp, hit to 50dp (Android's recommended minimum)
- [ ] Press-down feedback — visual state change on finger-down (tint or scale)
- [ ] Show/hide animation — fade + scale from center on open/close
- [ ] Review layout for mobile — consider alternative arrangements if radial is still awkward

### Feature: Android Lifecycle

- [ ] Handle `APP_CMD_TERM_WINDOW` / `APP_CMD_INIT_WINDOW` — surface re-creation without quitting
- [ ] Reset input FSM on `APP_CMD_RESUME`
- [ ] Save/restore scroll position and current chapter on pause/resume
- [ ] Fix bottom bar input corruption after resume

### Release Steps

- [ ] Reconfigure and build (all 3 platforms)
- [ ] Full test suite passes (≥ 70/72)
- [ ] Manual verification on Android
- [ ] Update `State.md`
- [ ] Bump version, tag

## Backlog (Future Releases)

### Partially Implemented

- **Copy Verse** — half-implemented; right-click/long-press copy exists in radial menu but more work needed (formatting, full verse selection UX)

### Phase 10 Remaining Items (deferred)

- [x] Java activity stub (`TheWordActivity.java`) for IME/splash — done
- [x] Soft keyboard integration for go-to dialog — done
- [ ] Complex IME composition (CJK, emoji) — would require hidden EditText + TextWatcher (future)
- [ ] Immersive mode (hide nav bar)
- [ ] Lifecycle save/restore (scroll position on pause/resume) — now part of v1.6.2
- [ ] WASM persistence via IDBFS
- [ ] `scripts/build-wasm.sh` for one-step WASM build
- [ ] libcurl as optional dependency
- [ ] Platform abstraction module (`src/core/Platform.h`)

### Feature Backlog

- Note System (details TBD)
- Custom Reading Plan System (details TBD)
- Bookmark System (expansion of highlight system, details TBD)
- Footnote display in reader
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
