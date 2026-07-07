# Release Plan

> Status: v1.6.0-alpha.1 in development | Last Updated: 2026-07-05

## Past Releases

### v1.5.0-alpha.2 (2026-07-05)

**Theme:** UI/UX Polish & Cross-Platform Verification (continued)

- [x] This is the final 1.5.0 pre-release. 1.5.0-stable will not ship — development moves to 1.6.0.
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

## Current Release: v1.6.0-alpha.1

**Theme:** Copy Verse + Code Quality

### Feature: Copy Verse
- [ ] Right-click or long-press on any verse → copies full verse text to clipboard
- [ ] Context menu shows "Copy Verse" button on non-highlighted text; adds "Copy" alongside existing highlight controls on highlighted text
- [ ] Verse text formatted as "Book Chapter:Verse text" (full verse, not section)

### Feature: Code Quality Refactor
- [ ] `UIManager::contextMenu`: raw `new`/`delete` → `std::unique_ptr<ContextMenu>`

### Release Steps
- [x] Version bump: `1.5.0-alpha.2` → `1.6.0-alpha.1`
- [ ] Reconfigure and build
- [ ] Full test suite passes (≥ 70/72)
- [ ] Desktop build clean
- [ ] Tag: `v1.6.0-alpha.1`

## Backlog (Future Releases)

### Phase 10 Remaining Items (deferred)
- [x] Java activity stub (`TheWordActivity.java`) for IME/splash — done
- [x] Soft keyboard integration for go-to dialog — done (raylib patch, basic char input via `getUnicodeChar()`)
- [ ] Complex IME composition (CJK, emoji) — would require hidden EditText + TextWatcher (future)
- [ ] Immersive mode (hide nav bar)
- [ ] Lifecycle save/restore (scroll position on pause/resume)
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
