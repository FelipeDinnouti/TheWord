# Release Plan

> Status: APKs built — ready for testing | Last Updated: 2026-07-05 (Android text input fix)

## Last Release: v1.4.2

**Theme:** MVP Completion Milestone

Just tag the current working state. No code changes.

- [x] Phase 13 Highlight Browser — data model + persistence + UI complete
- [x] All 72 unit tests passing
- [x] Desktop build clean
- [x] Bump CMakeLists.txt: `1.4.1` → `1.4.2`
- [x] Build: `cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build --parallel`
- [x] Tag: `git tag -am "v1.4.2" "v1.4.2"`

## Current Release: v1.5.0-alpha.1

**Theme:** UI/UX Polish & Cross-Platform Verification

### Design System
- [ ] Add accent color (`ACCENT_TEAL = #0EA5E9`) and corner radius (`PANEL_ROUNDING = 6.0f`) to `Theme.h`
- [ ] Add hover/press tint constants (`HOVER_DARKEN`, `PRESS_DARKEN`)
- [ ] Expand `components.h/cpp` with reusable primitives:
  - [ ] `DrawButton()` — rounded rect + label, handles hover/press states internally
  - [ ] `DrawPanel()` — rounded container with optional border
  - [ ] `DrawTextItem()` — selectable list item with hover/press
  - [ ] `DrawToggle()` — on/off switch with accent color
  - [ ] `DrawColorSwatch()` — extracted from current inline code
- [ ] Refactor all screens to use component primitives instead of raw `DrawRectangle()`

### Performance: Non-Blocking Chapter Navigation
- [x] **Android text input (GetCharPressed)** — patched raylib 5.0's `rcore_android.c` to fix two issues: (1) gamepad/keyboard source flag collision that swallowed keyboard events on some devices, and (2) missing `charPressedQueue` population via JNI `KeyEvent.getUnicodeChar()`. Search bar and dialogs now accept text input on Android. See `cmake/patches/raylib-5.0-android-char-input.patch` for details.
- [x] **Async chapter loading** — `LoadInitialChapter` now runs on a background thread via `std::async`; the screen switches to the reader immediately while the chapter loads in the background
- [x] **ChapterLoadedEvent** — new event decouples DocumentManager from Highlighter; context is set when the chapter finishes loading, not inline in the navigation event handler
- [x] **Graveyard pattern** — cancelled in-flight futures are moved to a graveyard and drained non-blockingly to prevent frame drops
- [x] **Startup preserved** — `App::Init()` uses `LoadInitialChapterSync()` so the initial chapter is ready before the first frame (behind the splash screen)

### Bug Fixes
- [x] **Chapter edge flickering** — 0.5px margin in `UpdateVisibleChapter()` boundary check
- [x] **Highlights lost on chapter reload** — per-chapter word IDs (deterministic, stable across reloads) + chapter-filtered highlight lookups
- [x] **Highlight browser navigation off-by-one** — same root cause (global word IDs); fixed by per-chapter IDs
- [x] **Corrupted DB** — `~/.theword/highlights.db` deleted (contained old global-ID references)

### Visual Feedback
- [x] **Selection tint during drag** — semi-transparent overlay follows finger/mouse across selected words
- [x] **Hover states** (desktop) — background darkens on mouse-over for: bottom bar buttons, menu items, book list rows, chapter grid cells, settings controls, context menu
- [x] **Press states** (desktop + mobile) — darker tint on press for all interactive elements
- [x] **Cursor changes** — hand cursor over clickable items
- [x] **Overlay fade-in** — center menu + credits alpha-fade in (100ms ease-out)

### Rounded Corners & Grid Redesign
- [x] Center menu panel
- [x] Context menu
- [x] Settings screen panels
- [x] Book list items
- [x] Chapter grid — remove cell borders and grey backgrounds; teal accent on selected cell only, rest as clean text on page background
- [x] Bottom bar
- [x] Scrollbar thumb (2px, subtle)

### Cross-Platform Verification
- [ ] **Diagnose chapter grid crash on mobile** — likely I/O or navigation stack issue, not input
- [ ] Verify: book list scroll + tap on mobile
- [ ] Verify: bottom bar prev/next + center menu on mobile
- [ ] Verify: settings screen all controls on mobile
- [ ] Verify: context menu (long-press) on mobile
- [ ] Verify: highlight creation (touch drag) on mobile
- [ ] Verify: go-to dialog keyboard on desktop
- [ ] Verify: all 4 screen sizes (phone, tablet, desktop, WASM)
- [x] Full test suite passes: `./build/theword_test` (72/72)

### Release Steps
- [x] Version bump: `1.4.2` → `1.5.0-alpha.1`
- [x] APK build: `scripts/build-android.sh` (x86_64 + arm64-v8a)
- [ ] Distribute APK to testers (~5 people)
- [ ] Gather feedback

## Backlog (Future Releases)

### Phase 10 Remaining Items
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
- `UIManager::contextMenu`: change from raw `new`/`delete` to `std::unique_ptr<ContextMenu>` (code quality)
- Bookmark System (expansion of highlight system, details TBD)
- Footnote display in reader
- **Copy Verse** — long-press or right-click on a verse to copy its full text to clipboard (full verse, not section)
- **Bible Version Switcher** — dropdown/selector in SettingsScreen to choose from enabled YouVersion IDs; persist choice; re-init `BibleClient` + `CompositeProvider` on change
- Search across books/chapters
- Dark / sepia theme
- Night mode
- Reading progress tracking
- Font selection (system fonts)
- Highlight export/import
- Performance profiling & optimization
