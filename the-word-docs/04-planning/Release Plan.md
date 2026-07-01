# Release Plan

> Status: Planning v1.5.0-beta.1 | Last Updated: 2026-07-01

## Current Release: v1.4.2

**Theme:** MVP Completion Milestone

Just tag the current working state. No code changes.

- [x] Phase 13 Highlight Browser — data model + persistence + UI complete
- [x] All 72 unit tests passing
- [x] Desktop build clean
- [ ] Bump CMakeLists.txt: `1.4.1` → `1.4.2`
- [ ] Build: `cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build --parallel`
- [ ] Tag: `git tag -am "v1.4.2" "v1.4.2"`

## Next Release: v1.5.0-beta.1

**Theme:** UI/UX Polish & Cross-Platform Verification

### Design System
- [ ] Add accent color (`ACCENT_TEAL = #0EA5E9`) and corner radius (`PANEL_ROUNDING = 6.0f`) to `Theme.h`
- [ ] Add hover/press tint constants (`HOVER_DARKEN`, `PRESS_DARKEN`)
- [ ] Add `TintForState()` helper to `components.h/cpp`
- [ ] Add `DrawRoundedPanel()` helper wrapping `DrawRectangleRounded`

### Visual Feedback
- [ ] **Selection tint during drag** — semi-transparent overlay follows finger/mouse across selected words
- [ ] **Hover states** (desktop) — background darkens on mouse-over for: bottom bar buttons, menu items, book list rows, chapter grid cells, settings controls, context menu
- [ ] **Press states** (desktop + mobile) — darker tint on press for all interactive elements
- [ ] **Cursor changes** — hand cursor over clickable items
- [ ] **Overlay fade-in** — center menu + credits alpha-fade in (100ms ease-out)

### Rounded Corners
- [ ] Center menu panel
- [ ] Context menu
- [ ] Settings screen panels
- [ ] Book list items
- [ ] Chapter grid cells
- [ ] Bottom bar
- [ ] Scrollbar thumb (2px, subtle)

### Toast Notifications
- [ ] `ShowToast(message, duration)` in UIManager
- [ ] Messages: "Highlight saved", "Highlight removed", "Preference saved"
- [ ] Rounded teal rectangle, white text, auto-fade

### Cross-Platform Verification
- [ ] **Fix chapter grid touch on mobile** — grid click uses `GetMousePosition()` which may not map correctly on Android touch; investigate and fix
- [ ] Verify: book list scroll + tap on mobile
- [ ] Verify: bottom bar prev/next + center menu on mobile
- [ ] Verify: settings screen all controls on mobile
- [ ] Verify: context menu (long-press) on mobile
- [ ] Verify: highlight creation (touch drag) on mobile
- [ ] Verify: go-to dialog keyboard on desktop
- [ ] Verify: all 4 screen sizes (phone, tablet, desktop, WASM)
- [ ] Full test suite passes: `./build/theword_test`

### Release Steps
- [ ] APK build: `scripts/build-android.sh`
- [ ] Distribute APK to testers (~5 people)
- [ ] Gather feedback

## Backlog (Future Releases)

### Phase 10 Remaining Items
- [ ] Java activity stub (`TheWordActivity.java`) for IME/splash
- [ ] Soft keyboard integration for go-to dialog
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
- Search across books/chapters
- Dark / sepia theme
- Night mode
- Reading progress tracking
- Font selection (system fonts)
- Highlight export/import
- Performance profiling & optimization
