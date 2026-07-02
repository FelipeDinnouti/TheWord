# Dynamic Item Layout for HighlightBrowserScreen

> **Target**: v1.5.0-alpha.1
> **Status**: Planned (not yet implemented)
> **Related**: Issue 1 (subtitle wrapping) from polish session

## Problem

The highlights screen currently uses fixed-height items (`itemH = max(48dp, controlSize*2 + 16dp)`). The subtitle (verse text) is drawn as a single `DrawTextEx` call — long text overflows off the right edge. While a `WrapText` helper was added, it still uses fixed item height with truncation to 2 lines, which clips or wastes space.

## Goal

- Subtitle text wraps to any number of lines with no truncation
- Each item's height is computed from its actual subtitle line count
- Scrolling uses pixel offsets, not item indices
- Same 20px gutters on left/right and consistent padding inside each item

## Design

### Item geometry (per item)

```
topPad=4dp
title    at relY + topPad,  fontSize=controlSize
gap=6dp
subLine0 at relY + topPad + titleH + gap,  fontSize=subSize(0.85*controlSize)
subLine1 at relY + topPad + titleH + gap + 1*subLineH
...
bottomPad=8dp
```

Total height: `topPad + controlSize + gap + numLines * subLineH + bottomPad`

Where `subLineH = subSize * 1.4` (font size × 1.4 line spacing).

### Data structure

```cpp
struct ItemLayout {
    float height;
    std::vector<std::string> subtitleLines;
};
```

Stored as `mutable std::vector<ItemLayout> layouts_` in the screen class.

### When layout is computed

Layout computation is **separate from `GetFilteredItems()`** because it needs screen width. A `RebuildLayouts(float textWrapWidth)` method is called from `Draw()`, with its own cache key:

| Trigger | Effect |
|---|---|
| `activeColorId_` changes | Full relayout |
| Highlights added/removed | Full relayout (rare — on user action) |
| Window resize (width change) | Full relayout |
| Scrolling | **No relayout** — cached |

`RebuildLayouts` compares `(cachedFilterColorId_, cachedHighlightsCount_, lastTextWrapWidth_)` against current state. On mismatch it runs `WrapText(subtitle, textWrapWidth, font_, subSize, 0)` for each filtered item and stores the resulting `ItemLayout` in `layouts_`.

### Drawing

```cpp
// In Draw():
// 1. Build layout if stale: RebuildLayouts(textWrapWidth)
// 2. totalHeight = sum of all layout heights
// 3. maxScroll = max(0, totalHeight - listH)
// 4. Clamp scrollY_
// 5. Walk each item:
//      relY = sum of heights of all previous items
//      screenY = listY + relY - scrollY_
//      if (screenY + layout.height < 0 || screenY > screenH) skip
//      draw title, draw each subtitle line from layout.subtitleLines
```

### Input handling

```cpp
// In HandleInput():
// Scroll wheel: scrollY_ = clamp(scrollY_ - wheel * uiScale_.dp(48), 0, maxScroll)
// Tap: walk layouts_, compute relY cumulatively,
//      check if mousePos.y falls within (listY + relY - scrollY_) .. (listY + relY + layout.height - scrollY_)
//      → find which item was tapped
```
Touch scroll via `ScrollEvent` subscription (same pattern as `BookListScreen`):
```cpp
eventBus_.On<ScrollEvent>(
    [this, alive = aliveGuard_](const ScrollEvent& e) {
    if (!*alive) return;
    // rebuild layout to get maxScroll, then clamp
    float textWrapWidth = GetScreenWidth() - uiScale_.dp(20) * 2;
    RebuildLayouts(textWrapWidth);
    float totalH = std::accumulate(...);
    float maxScroll = std::max(0.0f, totalH - listH);
    scrollY_ = std::clamp(scrollY_ - e.delta, 0.0f, maxScroll);
});
```

### Variables to replace/keep

| Old (remove) | New (add) |
|---|---|
| `float scrollOffset_` | `float scrollY_` |
| `int`-based item indexing | pixel-based hit testing |
| fixed `itemH` | per-item `layouts_[i].height` |
| `(void)` | `mutable std::vector<ItemLayout> layouts_` |
| `(void)` | `mutable float lastTextWrapWidth_ = 0` |
| `(void)` | `std::shared_ptr<bool> aliveGuard_` |

### Edge cases

- **Empty state**: Return early before layout, same as current code
- **1-line subtitle**: Height shrinks to single line, no wasted space
- **Many lines (5+)**: Item grows naturally, scroll moves to see it

### Files affected

| File | Change |
|---|---|
| `src/ui/HighlightBrowserScreen.h` | Replace `scrollOffset_` → `scrollY_`, add `ItemLayout` struct, `layouts_`, `lastTextWrapWidth_`, `aliveGuard_`, `RebuildLayouts()` declaration |
| `src/ui/HighlightBrowserScreen.cpp` | Add `RebuildLayouts()` method, `ScrollEvent` subscription in constructor, `aliveGuard_` in destructor, rewrite `Draw()` + `HandleInput()` for pixel scrolling + cached layout |

### Verification

- [ ] All 72 existing tests still pass
- [ ] ASan build: no new memory errors
- [ ] Integration test: navigate to highlights screen, verify scroll works
- [ ] Visual: subtitle wraps without truncation, no overlap between items
