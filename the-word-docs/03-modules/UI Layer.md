# UI Layer

> Status: Updated for Phase 9 Sprint 3 | Last Updated: 2026-06-22
> Phase 9 plan: See `04-planning/Progress Tracking.md`
> Sprint 3 (Navigation & Settings) complete — 64/64 tests passing

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

## InputHandler

Extracted in Phase 9 Sprint 1 from `main.cpp`. Lives in `src/input/InputHandler.h/cpp`.

### Responsibilities
- Mouse wheel input → scroll velocity with friction decay
- Keyboard (Up/Down) → scroll velocity
- Left-click finite state machine: **Idle → Pending → Dragging/LongPress** (500ms hold → context menu)
  - Hold < 500ms + release → single-word highlight
  - Move > 10px while held → drag selection
  - Hold > 500ms without significant move → long-press context menu
- Right-click → immediate context menu on highlighted word (no pending state)
- Keyboard shortcuts: `G` toggles go-to dialog, `S` toggles settings, `Escape` dismisses any active dialog
- Dialog routing: when go-to or settings dialog is active, normal input (scroll, click, selection) is suppressed and input is routed to the active dialog handler
- When go-to dialog active: routes `GetCharPressed()` / Enter / Tab / Backspace / Up / Down to `UIManager::handleGoToKeyboardInput()`
- When settings active: routes left-click to `UIManager::handleSettingsClick()`
- Window resize → re-layout via `LayoutEngine` and `DocumentManager`, with scroll-fraction anchoring

### Interface
```cpp
class InputHandler {
public:
    InputHandler(DocumentManager& docManager, Highlighter& highlighter,
                 LayoutEngine& layoutEngine, UIManager& uiManager, float contentTop);
    void handleInput(float deltaTime);
};
```

### Dependencies
- `DocumentManager` (scroll, hit-test)
- `Highlighter` (selection lifecycle)
- `UIManager` (dialog routing, keyboard shortcut dispatch)
- `LayoutEngine` (width change on resize)
- Raylib (`GetMouseWheelMove`, `IsKeyDown`, `IsMouseButton*`, `IsWindowResized`)

## UIManager

Extracted in Phase 9 Sprint 1. Lives in `src/renderer/UIManager.h/cpp`.

### Responsibilities
- Draw top bar with chapter title (`drawTopBar`)
- `getContentTop()` returns the top bar height (60px) used by Renderer for document offset
- Context menu on long-press/right-click: single-row popup with "Del" button (red) + 5 pastel color swatches (side-by-side)
- Context menu click handling: delete highlight, recolor highlight, or dismiss on outside/Escape
- Go-to dialog: text input field with auto-complete (matches book code or full name, case-insensitive, up to 5 suggestions), keyboard navigation (Enter loads chapter, Tab auto-completes selected, Up/Down cycle, Backspace deletes, Escape dismisses)
- Settings panel: modal overlay with font size A–/A+ buttons (12–36 range), USFM/Online version toggle (hidden when no CompositeProvider), active color swatch selector with black border on selected
- Settings apply: font size → `LayoutEngine::setFontSize` + `invalidateCache` + `Renderer::setFontSize` + `DocumentManager::invalidateLayouts` + persist; version toggle → `CompositeProvider::setPrimary` + reload current chapter + persist; color swatch → `Highlighter::setActiveTypeId` + persist

### Interface
```cpp
class UIManager {
public:
    UIManager(const Font& headingFont, float headingSize, Highlighter& highlighter,
              DocumentManager& docManager, LayoutEngine& layoutEngine,
              Renderer& renderer, PersistenceManager& persistence,
              ChapterProvider& onlineProv, ChapterProvider& offlineProv,
              CompositeProvider* compositeProv, float initialFontSize = 24.0f,
              bool initialVersionOnline = false);
    float getContentTop() const;
    float getFontSize() const;
    void drawTopBar(const std::string& chapterTitle);
    void drawContextMenu();
    void drawSettingsPanel();
    void drawGoToDialog();
    void showContextMenu(Vector2 position, int highlightId, int typeId);
    void hideContextMenu();
    bool isContextMenuActive() const;
    bool handleContextMenuClick(Vector2 pos);
    void toggleGoToDialog();
    void dismissGoToDialog();
    bool isGoToDialogActive() const;
    void handleGoToKeyboardInput();
    void toggleSettings();
    void dismissSettings();
    bool isSettingsActive() const;
    void handleSettingsClick(Vector2 pos);
    void dismissActiveDialog();
};
```

### Dependencies
- `Highlighter` (active color, highlight manipulation)
- `DocumentManager` (load chapter on go-to, invalidate layouts on font change)
- `LayoutEngine` (set font size, cache invalidation)
- `Renderer` (set font size)
- `PersistenceManager` (save/load preferences)
- `ChapterProvider` (online + offline refs for version toggle)
- `CompositeProvider` (setPrimary for version switching; null when no online API available)

### Design Note: Renderer Extraction from main.cpp

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

## Keyboard Shortcuts

| Key | Context | Action |
|-----|---------|--------|
| `G` | Any (no dialog active) | Open go-to dialog |
| `S` | Any (no dialog active) | Open settings panel |
| `Escape` | Go-to dialog active | Dismiss go-to dialog |
| `Escape` | Settings active | Dismiss settings |
| `Escape` | Context menu active | Dismiss context menu |
| `Enter` | Go-to dialog | Parse input and navigate to chapter |
| `Tab` | Go-to dialog | Auto-complete selected suggestion |
| `Up` / `Down` | Go-to dialog | Cycle through suggestions |
| `Backspace` | Go-to dialog | Delete last character |
| Arrow keys | Any | Scroll text |

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
