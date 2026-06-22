# UI Layer

> Status: Implemented (extracted in Phase 4) | Last Updated: 2026-06-22 (Phase 9 checklist)
> Phase 9 plan: See `04-planning/Progress Tracking.md`

## Overview

The UI layer brings together all lower layers with a clean user interface and input handling.

## Components

### Renderer

The top-level render coordinator. Responsibilities:
- Query the document manager for visible spans
- Query the highlighter for highlights in visible range
- Draw text with segment-aware formatting (headings centered, poetry indented, etc.)
- Draw highlight rectangles behind text
- Draw UI elements (title bar, scrollbar, controls)

### InputHandler

Translates Raylib input events into document actions:
- Mouse wheel / keyboard → scroll
- Mouse click → hit detection (word lookup via LayoutEngine)
- Mouse drag → selection range
- Touch gestures → scroll and select

### UIManager

Manages UI state and rendering:
- Chapter title display (top bar)
- Font size controls
- Navigation controls (go to book/chapter)
- Debug overlay (FPS, scroll position, word count)

## Layout

```
┌──────────────────────┐
│  Gênesis 1            │  <- 40px top bar (chapter title)
├──────────────────────┤
│   A Criação           │  <- Section heading (centered, bold)
│                       │
│ No princípio criou    │  <- Verse text (left-aligned)
│ Deus os céus e a      │
│ terra.                │
│                       │
│   Então Deus disse:   │
│     — Façamos o ser   │  <- Poetry (indented)
│       humano          │
│                       │
│                       │
├──────────────────────┤
│  [Controls]  [Font]   │  <- Optional bottom bar
└──────────────────────┘
```

## Window Resize Handling

When the window width changes:
1. Invalidate all cached layouts
2. Re-layout all loaded chapters
3. Adjust scroll position to keep visible content anchored

## Segment-Aware Rendering

The Renderer handles each `Span` differently based on its originating segment type:

- **VerseText spans**: Drawn left-aligned with `DrawTextEx()`
- **PoetryLine spans**: Drawn at the pre-computed indent position
- **ParagraphBreak**: Only affects positioning (vertical gap), no draw

**Current (Phase 8):** SectionHeading, ChapterLabel, and BookTitle all render identically — centered, headingFont at 1.3×, DARKGRAY.

**Planned for Phase 9:** Heading differentiation — each heading type gets its own distinct style:

| Type | Font | Size | Color | Alignment |
|------|------|------|-------|-----------|
| **BookTitle** | `headingFont` | `fontSize × 1.6` | `BLACK` | Centered |
| **SectionHeading** | `headingFont` | `fontSize × 1.3` | `DARKGRAY` | Centered |
| **ChapterLabel** | `headingFont` | `fontSize × 1.6` | `(80, 80, 80)` | Centered |

This requires splitting the grouped `case` in `Renderer::drawSpan()` into three separate branches. No layout engine changes needed — all three are already positioned as centered headings by the LayoutEngine.

## Design Note: Renderer Extraction from main.cpp

The current render loop lives entirely in `main.cpp`. Before adding highlight rendering or UI controls, the renderer should be extracted into its own module.

**Current `Renderer` class:**
```cpp
class Renderer {
public:
    Renderer(const Font& bodyFont, const Font& headingFont, float contentTop, float fontSize);
    void drawFrame(float scrollY, float totalHeight, float viewportHeight,
                   const std::vector<std::pair<Span, float>>& docSpans,
                   const std::string& chapterTitle);
    void drawScrollbar(float scrollY, float totalHeight, float viewportHeight);
    void drawFpsCounter(int x, int y);
    float getContentTop() const;
private:
    Font bodyFont;
    Font headingFont;
    float contentTop;
    float fontSize;
    float headingSize;
    void drawSpan(const Span& span, float screenY);
};
```

Renderer uses **two font atlases**: `bodyFont` (24px atlas, 1:1 for verse text) and `headingFont` (31px atlas, 1:1 for section headings, chapter labels, book titles). Both use `TEXTURE_FILTER_POINT` for pixel-sharp rendering. `drawSpan` selects the appropriate font by `span.type`.

## Android Considerations

When porting to Android (Phase 10):
- The mouse wheel and keyboard input code must be replaced with touch gesture recognition.
- Raylib's Android entry point is `android_app_init()` instead of `main()`.
- A separate CMake toolchain file targeting the Android NDK is required.
- Lifecycle (pause/resume) must save/restore scroll position and highlights.

**Recommended intermediate step:** Build a WebAssembly target via Emscripten first. This reuses the desktop codebase while forcing touch event handling and a different build pipeline — a smaller jump than going straight to Android.

## Files

- `src/renderer/Renderer.h/cpp`
- `src/renderer/UIManager.h/cpp`
- `src/input/InputHandler.h/cpp`
