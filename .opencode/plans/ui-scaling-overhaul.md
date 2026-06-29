# UI Scaling Overhaul — Implementation Plan

> **Status**: Planning Phase | **Target**: Phase 10 (Mobile/Android) refinement

## Problem

All UI dimensions are raw pixel values. On high-DPI Android displays (Moto G56: 1080×2400, ≈2.5× density), a `60.0f` header bar is 60 physical pixels = ~24 density-independent pixels (dp) — tiny. Android Material Design recommends **48dp minimum** for touch targets.

**Root cause**: No unified scaling abstraction. `scale_` exists in 3 of 7 screens but is used inconsistently or not at all.

## Strategy

Create a single `UIScale` object that encapsulates DPI scaling and screen-relative sizing. Replace `float scale` with `const UIScale&` everywhere. Use `uiScale.dp(N)` for DPI-independent sizing and `uiScale.vw/vh()` for screen-relative sizing.

### The Pattern

```cpp
// Density-independent pixels → physical pixels
float dp(float n) const { return n * dpiScale_; }

// Viewport-relative (percentage of screen)
float vw(float percent) const { return screenW_ * percent / 100.0f; }
float vh(float percent) const { return screenH_ * percent / 100.0f; }

// Screen-fitted: use percent of screen, but cap at dpMax
float fitScreen(float vwPercent, float dpMax) const {
    return std::min(vw(vwPercent), dp(dpMax));
}
```

Font-size limiting is done by callers at point of use:
```cpp
float textHeight = MeasureTextEx(font, label, fontSize, 1).y;
float elementHeight = std::max(uiScale.dp(48), textHeight + uiScale.dp(12));
```

---

## Steps

### Step 1 — Create `src/core/UIScale.h`

Header-only struct. No raylib dependency in the header (callers use raylib types).

```cpp
#ifndef UISCALE_H
#define UISCALE_H

#include <algorithm>

namespace theword::core {

struct UIScale {
    float dpiScale;    // physical DPI / 160
    float screenW;     // GetScreenWidth()  (updated on resize)
    float screenH;     // GetScreenHeight() (updated on resize)

    UIScale(float dpi, float w, float h)
        : dpiScale(dpi), screenW(w), screenH(h) {}

    void OnResize(float w, float h) { screenW = w; screenH = h; }

    float dp(float n) const { return n * dpiScale; }
    float vw(float percent) const { return screenW * percent / 100.0f; }
    float vh(float percent) const { return screenH * percent / 100.0f; }

    float fitScreen(float vwPct, float dpMax) const {
        return std::min(vw(vwPct), dp(dpMax));
    }
};

} // namespace theword::core
#endif
```

**Verification**: build + tests pass (nothing references it yet).

---

### Step 2 — Wire UIScale into `App::Init()`

In `App::Init()` after `platform::Init()` sets `scale_`:

```cpp
// After line 65: renderW = GetScreenWidth(); renderH = GetScreenHeight();
uiScale_.emplace(scale_, (float)renderW, (float)renderH);
```

Replace `scale_` with `uiScale_.dpiScale` where the raw value is still needed (font loading). Pass `*uiScale_` to all screen constructors instead of `scale_`.

Add `UIScale` member to App:
```cpp
std::optional<UIScale> uiScale_;  // or just UIScale with default
```

Update screen creations in `App::Init()` and `App::Run()` (S/A key shortcuts) to pass `*uiScale_` instead of `scale_`.

Handle resize in `WireEvents` → `ResizeEvent`:
```cpp
eventBus_.On<ResizeEvent>([this](const ResizeEvent& e) {
    uiScale_->OnResize((float)e.width, (float)e.height);
    // ... rest of resize handling ...
});
```

**Files changed**:
- `src/app/App.h` — add `UIScale uiScale_;` member
- `src/app/App.cpp` — create UIScale, pass to screens, handle resize
- `src/core/UIScale.h` — already created

**Verification**: build + tests pass. App runs identically (screens still ignore UIScale internally, only signature changed).

---

### Step 3 — CreditsOverlay (easiest, ~10 values)

| File | Change |
|------|--------|
| `CreditsOverlay.h` | Replace `float fontSize_` with `const UIScale& uiScale_`. Add `const Font& font_` (already there) |
| `CreditsOverlay.cpp` | Replace all hardcoded constants with `uiScale_` methods |

**Constant changes**:

| Old | New | Rationale |
|-----|-----|-----------|
| `PANEL_WIDTH = 360.0f` | `uiScale_.fitScreen(90, 400)` | 90% screen width, max 400dp |
| `PANEL_HEIGHT = 200.0f` | Content-determined | ~6 lines of text + padding |
| `PADDING = 20.0f` | `uiScale_.dp(16)` | 16dp standard padding |
| `fontSize_ * 0.7f` label | `uiScale_.font(0.7f)` or keep as-is (already font-relative) | UI text size |
| `fontSize_ * 0.55f` small | `uiScale_.font(0.5f)` | |
| `labelSize + 12.0f` | `labelSize + uiScale_.dp(12)` | Gap after title |
| `smallSize + 8.0f` | `smallSize + uiScale_.dp(8)` | Standard line gap |
| Close button `-40.0f, +40.0f` | `uiScale_.dp(24)` from edges | 24dp close button hit area |

Parentheses from `(screenW - PANEL_WIDTH) / 2` stay — they already center the panel correctly.

**Verification**: build + tests pass. Run app, press A, verify overlay is properly sized.

---

### Step 4 — CenterMenu (simple list, ~8 values)

| File | Change |
|------|--------|
| `CenterMenu.h` | Replace `float scale_` with `const UIScale& uiScale_` |
| `CenterMenu.cpp` | Use `uiScale_` methods |

**Constant changes**:

| Old | New |
|-----|-----|
| `MENU_WIDTH = 280.0f` | `uiScale_.fitScreen(85, 320)` |
| `MENU_ITEM_HEIGHT = 44.0f` | `std::max(uiScale_.dp(48), textHeight + uiScale_.dp(12))` |
| `MENU_PADDING = 12.0f` | `uiScale_.dp(12)` |

**Verification**: build + tests pass. Run app, press G, verify menu items are tappable.

---

### Step 5 — BookListScreen (list + search)

| File | Change |
|------|--------|
| `BookListScreen.h` | Add `const UIScale& uiScale_` member. Drop `fontSize_` if unused for scaling (keep for text size). |
| `BookListScreen.cpp` | Replace hardcoded values |

**Constant changes**:

| Old | New |
|-----|-----|
| `HEADER_HEIGHT = 60.0f` | `uiScale_.dp(48)` |
| `SEARCH_HEIGHT = 40.0f` | `uiScale_.dp(44)` |
| `ITEM_HEIGHT = 36.0f` | `std::max(uiScale_.dp(44), textHeight + uiScale_.dp(10))` |
| `BACK_AREA_WIDTH = 80.0f` | `uiScale_.dp(56)` |
| `searchBox.x = 12.0f` | `uiScale_.dp(12)` |
| `searchBox.width = screenW - 24.0f` | `screenW - uiScale_.dp(12) * 2` |
| `textX = 6.0f, textY = 4.0f` | `uiScale_.dp(6), uiScale_.dp(4)` |
| Book label `20.0f` from left | `uiScale_.dp(16)` |
| Book label Y `itemY + 8.0f` | `itemY + uiScale_.dp(8)` |

**Verification**: build + tests pass. Run app, navigate to BookList, verify items are properly sized.

---

### Step 6 — ChapterGridScreen (grid)

| File | Change |
|------|--------|
| `ChapterGridScreen.h` | Add `const UIScale& uiScale_` member |
| `ChapterGridScreen.cpp` | Replace hardcoded values |

**Constant changes**:

| Old | New |
|-----|-----|
| `HEADER_HEIGHT = 60.0f` | `uiScale_.dp(48)` |
| `GRID_PADDING = 16.0f` | `uiScale_.dp(12)` |
| `CELL_WIDTH = 60.0f` | `uiScale_.dp(56)` |
| `CELL_HEIGHT = 44.0f` | `uiScale_.dp(48)` |
| `CELL_GAP = 8.0f` | `uiScale_.dp(8)` |
| `BACK_AREA_WIDTH = 80.0f` | `uiScale_.dp(56)` |

**Verification**: build + tests pass. Run app, navigate to ChapterGrid, verify cells are tappable and grid fits screen.

---

### Step 7 — SettingsScreen (complex, ~25 values)

| File | Change |
|------|--------|
| `SettingsScreen.h` | Replace `float scale_` with `const UIScale& uiScale_` |
| `SettingsScreen.cpp` | Replace all layout constants |

**Constant changes**:

| Old | New |
|-----|-----|
| `HEADER_HEIGHT = 60.0f` | `uiScale_.dp(48)` |
| `ROW_Y1 = 100.0f` | `uiScale_.dp(100)` or `HEADER_HEIGHT + uiScale_.dp(40)` |
| `ROW_GAP = 50.0f` | `uiScale_.dp(48)` |
| `LABEL_X = 30.0f` | `uiScale_.dp(16)` |
| `FONT_BTN_W = 36.0f` | `uiScale_.dp(36)` |
| `FONT_BTN_H = 30.0f` | `uiScale_.dp(30)` |
| `FONT_DEC_X = 140.0f` | `uiScale_.dp(140)` |
| `FONT_VAL_X = 190.0f` | `uiScale_.dp(190)` |
| `FONT_INC_X = 240.0f` | `uiScale_.dp(240)` |
| `SRC_BTN_W = 80.0f` | `uiScale_.dp(80)` |
| `SRC_BTN_H = 30.0f` | `uiScale_.dp(30)` |
| `SRC_USFM_X = 140.0f` | `uiScale_.dp(140)` |
| `SRC_ONLINE_X = 230.0f` | `uiScale_.dp(230)` |
| `SWATCH_SIZE = 28.0f` | `uiScale_.dp(28)` |
| `SWATCH_GAP = 8.0f` | `uiScale_.dp(8)` |
| `COLOR_START_X = 140.0f` | `uiScale_.dp(140)` |
| `BACK_AREA_WIDTH = 80.0f` | `uiScale_.dp(56)` |
| `+10.0f` inside buttons | `uiScale_.dp(10)` |
| `+2.0f` text Y | `uiScale_.dp(2)` |
| `+20.0f` USFM text X | `uiScale_.dp(20)` |
| `+25.0f` API text X | `uiScale_.dp(25)` |

**Verification**: build + tests pass. Run app, press S, verify settings rows are properly sized and font +/- buttons work.

---

### Step 8 — `components.h/cpp` (DrawHeaderBar)

| File | Change |
|------|--------|
| `components.h` | Add `const UIScale&` parameter to `DrawHeaderBar` |
| `components.cpp` | Use `uiScale_` for bar height and back arrow position |
| Callers (BookListScreen, ChapterGridScreen, SettingsScreen) | Pass `uiScale_` to `DrawHeaderBar` |

**Constant changes**:

| Old | New |
|-----|-----|
| `barHeight = 60.0f` | `uiScale_.dp(48)` |
| `12.0f` back arrow X | `uiScale_.dp(8)` |
| `(barHeight - labelSize) / 2` | Already dynamically computed — keep |

**Verification**: build + tests pass. Run app, navigate screens, verify header bars are properly sized.

---

### Step 9 — ReaderScreen (already partially fixed)

Convert remaining hardcoded values:

| Old | New |
|-----|-----|
| `15.0f` ("<" arrow X) | `uiScale_.dp(12)` |
| `-25.0f` (">" arrow X from right) | `screenW - uiScale_.dp(12)` |
| `SHOW_HIDE_THRESHOLD = 30.0f` | `uiScale_.dp(30)` |
| `hitArea = 50 * scale_` | `uiScale_.dp(56)` (standard touch target) |
| `bottomBarHeight_ = 50 * scale_` | `std::max(uiScale_.dp(48), textHeight + uiScale_.dp(16))` |
| `bottomMargin_ = 20 * scale_` | `uiScale_.dp(20)` |

**Verification**: build + tests pass. Run app, bottom bar properly sized and positioned above nav bar.

---

### Step 10 — Renderer scrollbar

In `Renderer::DrawScrollbar()`:

| Old | New |
|-----|-----|
| `GetScreenWidth() - 8` | `GetScreenWidth() - (int)(8 * scale)` |
| `6` (width) | `(int)(6 * scale)` |
| `MIN_SCROLLBAR_HEIGHT = 20.0f` | `20.0f * scale` |

Note: Renderer doesn't have `scale_` directly. Options:
1. Pass UIScale to Renderer (but it's for UI not doc rendering)
2. Pass raw `scale` as a new ctor parameter (simplest)
3. Compute from bodySize_ / fontSize (fragile)

→ Option 2: add `float dpiScale_` to Renderer, set from `scale_` in App.

**Verification**: build + tests pass. Run app, scrollbar is properly sized on Android.

---

### Step 11 — Delete dead code

Files no longer compiled or referenced:
- `src/renderer/SettingsPanel.h` / `.cpp`
- `src/renderer/GoToDialog.h` / `.cpp`
- `src/renderer/AboutOverlay.h` / `.cpp`

Remove from CMakeLists.txt if present (currently NOT present — they're already orphaned). Simply delete the files.

**Verification**: build + tests pass.

---

### Step 12 — Bump version + tag

Bump MINOR version in `CMakeLists.txt` (new feature: cross-platform UI scaling system):
```
project(theword VERSION 1.3.0 LANGUAGES C CXX)
```

---

## Summary of all file changes

| File | Action | Notes |
|------|--------|-------|
| `src/core/UIScale.h` | **Create** | Header-only scaling abstraction |
| `src/app/App.h` | Modify | Add `UIScale uiScale_` member |
| `src/app/App.cpp` | Modify | Create UIScale, pass to screens, handle resize |
| `src/ui/CreditsOverlay.h` | Modify | Replace float with `const UIScale&` |
| `src/ui/CreditsOverlay.cpp` | Modify | ~10 constant replacements |
| `src/ui/CenterMenu.h` | Modify | Replace `float scale_` with `const UIScale&` |
| `src/ui/CenterMenu.cpp` | Modify | ~8 constant replacements |
| `src/ui/BookListScreen.h` | Modify | Add `const UIScale&` member |
| `src/ui/BookListScreen.cpp` | Modify | ~12 constant replacements |
| `src/ui/ChapterGridScreen.h` | Modify | Add `const UIScale&` member |
| `src/ui/ChapterGridScreen.cpp` | Modify | ~8 constant replacements |
| `src/ui/SettingsScreen.h` | Modify | Replace `float scale_` with `const UIScale&` |
| `src/ui/SettingsScreen.cpp` | Modify | ~25 constant replacements |
| `src/ui/components.h` | Modify | Add `const UIScale&` to `DrawHeaderBar` signature |
| `src/ui/components.cpp` | Modify | ~2 constant replacements |
| `src/ui/ReaderScreen.h` | Modify | Replace `float scale_` with `const UIScale&` |
| `src/ui/ReaderScreen.cpp` | Modify | ~6 constant replacements |
| `src/renderer/Renderer.h` | Modify | Add `float dpiScale_` member for scrollbar scaling |
| `src/renderer/Renderer.cpp` | Modify | Scrollbar position/width use `dpiScale_` |
| `src/renderer/SettingsPanel.h` | **Delete** | Dead code |
| `src/renderer/SettingsPanel.cpp` | **Delete** | Dead code |
| `src/renderer/GoToDialog.h` | **Delete** | Dead code |
| `src/renderer/GoToDialog.cpp` | **Delete** | Dead code |
| `src/renderer/AboutOverlay.h` | **Delete** | Dead code |
| `src/renderer/AboutOverlay.cpp` | **Delete** | Dead code |
| `CMakeLists.txt` | Modify | Bump version → 1.3.0 |
| `the-word-docs/04-planning/Progress Tracking.md` | Modify | Mark Phase 10 scaling items complete |
| `AGENTS.md` | Modify | Add UIScale note under conventions |

## Verification after each step

```bash
cmake --build build --parallel 2>&1 | tail -5
./build/theword_test 2>&1 | tail -3
```

Manual smoke test after all steps: launch app, verify all screens look correct on desktop. Then build Android APK and test on device.

## Rollback

Each step is a single commit. `git checkout HEAD -- <files>` reverts any step.

## Android dp values reference

| dp value | Physical size @ 2.5× | When to use |
|----------|----------------------|-------------|
| 48dp | 120px | Minimum touch target (Material Design) |
| 56dp | 140px | Back button hit area, grid cells |
| 44dp | 110px | Search bar, dense list items |
| 36dp | 90px | Small buttons (font +/-) |
| 28dp | 70px | Color swatches |
| 16dp | 40px | Standard padding (content edges) |
| 12dp | 30px | Tight padding, group spacing |
| 8dp | 20px | Small gaps between elements |
