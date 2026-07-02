# v1.5.0-alpha.1 Implementation Plan

> Target: v1.5.0-alpha.1 — UI/UX Polish & Cross-Platform Verification
> Based on: `the-word-docs/04-planning/Release Plan.md`
> Status: Planning Phase

## Phase 1 — Design System: Theme & Component Primitives ✅

**Goal:** Build the reusable component library so screens stop painting raw raylib rectangles. Components use immediate mode (check `GetMousePosition()` at draw time) — no external state needed.

### Steps

- [x] **Theme.h additions**
  - `ACCENT_TEAL = #0EA5E9` → `{14, 165, 233, 255}`
  - `PANEL_ROUNDING = 6.0f` (pixel radius, converted to roundness proportion at draw time)
  - `HOVER_DARKEN = 0.85f` and `PRESS_DARKEN = 0.70f` multipliers
  - `Darken(Color, float)` helper function

- [x] **Build `DrawButton(Rectangle, text, font, fontSize, enabled, textColor, bgColor) → bool`**
  - Rounded rect with hover/press tint (disabled state: grayed out)
  - Label centered, returns `true` on click release

- [x] **Build `DrawPanel(Rectangle, bgColor, borderColor, rounding, borderThick)`**
  - Rounded container with optional border

- [x] **Build `DrawTextItem(Rectangle, text, font, fontSize, selected, textColor, selTextColor) → bool`**
  - Selectable row with hover/press/selected visual states

- [x] **Build `DrawToggle(Rectangle, accentColor, bool& value) → bool`**
  - On/off pill switch; toggles `value` internally on click

- [x] **Build `DrawColorSwatch(Rectangle, color, selected) → bool`**
  - Rounded color square; black border when selected, clickable

### Verification
- [x] Code compiles clean on desktop
- [x] All 72 existing tests still pass

---

## Phase 2 — Screen Refactoring: Rounded Corners & Grid Redesign ✅

**Goal:** Every screen uses the new component primitives. Chapter grid gets its visual redesign (clean, no borders/grey backgrounds).

### Steps

- [x] **CenterMenu** → `DrawPanel` for background, `DrawTextItem` for each menu item
- [x] **ContextMenu** → `DrawPanel` for menu body, `DrawButton` for "Del", `DrawColorSwatch` for color swatches
- [x] **SettingsScreen** → `DrawButton` for font +/- and source buttons, `DrawColorSwatch` for color picker
- [x] **BookListScreen** → `DrawTextItem` for each book row
- [x] **ChapterGridScreen** — redesign:
  - Removed cell borders and grey backgrounds
  - Teal accent (`ACCENT_TEAL`) with subtle background tint + rounded rect on selected cell only
  - Clean text on page background for unselected cells
- [x] **BottomBar** (ReaderScreen) — `<` and `>` replaced with proper `DrawButton`s with hit targets
- [x] **CreditsOverlay** — `DrawPanel` for content background
- [x] **HighlightBrowserScreen** — `DrawColorSwatch` for filter swatches
- [x] **Scrollbar thumb** — rounded via `DrawRectangleRounded` (2px radius)

### Verification
- [x] Build runs without crash (0 warnings)
- [x] All 72 existing tests still pass

---

## Phase 3 — Visual Feedback Polish ✅

**Goal:** Interactive feel — selection tint, cursor changes, fade-in animations.

### Steps

- [x] **Selection tint during drag** — exposed `IsSelecting()`, `GetSelectionStart()`, `GetSelectionEnd()` on `Highlighter`; `ReaderScreen::Draw()` draws light blue translucent rects over spans within the selection range
- [x] **Cursor changes**:
  - `SetMouseCursor(MOUSE_CURSOR_POINTING_HAND)` added inside `DrawButton`, `DrawTextItem`, `DrawToggle`, `DrawColorSwatch` when hovered
  - `SetMouseCursor(MOUSE_CURSOR_IBEAM)` in `ReaderScreen::Draw()` when mouse is over document content area
  - `SetMouseCursor(MOUSE_CURSOR_DEFAULT)` at start of each frame in `App::Run()`
- [x] **Overlay fade-in** — `CenterMenu` & `CreditsOverlay` store `showTime_` (set via `GetTime()` in constructor), compute `fadeAlpha = clamp(elapsed / 0.1f, 0, 1)`, and fade all drawn colors with `Fade()` (overlay bg, panel bg/border, text)

### Verification
- [x] Build clean (0 warnings)
- [x] All 72 tests pass

---

## Phase 4 — Cross-Platform Verification

**Goal:** All 4 targets (phone, tablet, desktop, WASM) work correctly.

### Steps

- [ ] **Diagnose chapter grid crash on mobile** — add debug logging, reproduce, fix (likely I/O or navigation stack issue)
- [ ] Verify: book list scroll + tap on mobile
- [ ] Verify: bottom bar prev/next + center menu on mobile
- [ ] Verify: settings screen all controls on mobile
- [ ] Verify: context menu (long-press) on mobile
- [ ] Verify: highlight creation (touch drag) on mobile
- [ ] Verify: go-to dialog keyboard on desktop
- [ ] Verify: all 4 screen sizes (phone, tablet, desktop, WASM)
- [ ] Full test suite passes: `./build/theword_test`

---

## Phase 5 — Release

**Goal:** v1.5.0-alpha.1 tagged and distributed.

### Steps

- [ ] Bump `CMakeLists.txt` → `1.5.0-alpha.1`
- [ ] `cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build --parallel`
- [ ] `./build/theword` smoke test
- [ ] `./scripts/build-android.sh` → APK
- [ ] Distribute APK to ~5 testers
- [ ] `git tag -am "v1.5.0-alpha.1" "v1.5.0-alpha.1"`
- [ ] Gather feedback

---

## Dependency Chain

```
Phase 1 ──→ Phase 2 ──→ Phase 3 ──→ Phase 4 ──→ Phase 5
(primitives)  (refactor)   (polish)    (verify)    (ship)
```

Each phase is blocking — the next phase depends on the previous being complete.
