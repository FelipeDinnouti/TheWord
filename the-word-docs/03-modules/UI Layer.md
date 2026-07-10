# UI Layer

> Status: Updated for Phase 13 (in progress) | Last Updated: 2026-06-29
> Phase 11 architectual redesign: navigation stack replaces single-screen overlays

## Overview

The UI layer is organized as a **navigation stack** with a **bottom bar** on the root Reader screen. All screens are full-window with a header bar and back navigation. The design philosophy is documented in `02-architecture/UI Philosophy.md`.

## Navigation Model

```
Reader (root screen)
  ├── tap book code → Center Menu (centered dialog)
  │     ├── Books → BookList → ChapterGrid → Reader (at new chapter)
  │     ├── Settings → Settings → Reader
  │     ├── Highlights → HighlightBrowser → Reader
  │     └── Credits → Credits overlay → Reader
  └── ◄/► or ←/→ keys → Reader (at prev/next chapter)
```

Back navigation from any pushed screen: ← Back button (top-left), swipe-right gesture, or Escape key.

## Screen Layout

```
┌──────────────────────┐
│  ← Back     Title     │  <- Header bar (pushed screens only)
├──────────────────────┤
│                       │
│   (screen content)    │
│                       │
│                       │
├──────────────────────┤
│  ◄   GEN 1   ►        │  <- Bottom bar (Reader only, shows on scroll-up)
└──────────────────────┘
```

The Reader screen has no header bar. The bottom bar replaces the old top bar for showing the current chapter reference.

## Components

### Navigation Stack

Manages screen lifecycle: push, pop, current screen tracking, drawing order.

```cpp
class NavigationStack {
public:
    void Push(std::unique_ptr<Screen> screen);   // Push a new screen (becomes active)
    void Pop();                                   // Pop active screen, return to previous
    void PopAll();                                // Pop to root (Reader)
    Screen* GetActive();                          // Currently visible screen
    void DrawActive();                            // Draw current screen
    bool HandleInput();                           // Route input to active screen
    bool IsOnRoot() const;                        // Is Reader the active screen?
};
```

### Screens

Every screen implements a common interface:

```cpp
class Screen {
public:
    virtual ~Screen() = default;
    virtual void Draw() = 0;                        // Render the screen
    virtual bool HandleInput(float deltaTime) = 0;  // Return true if input was consumed
    virtual const char* GetTitle() const = 0;       // For header bar display
};
```

Concrete screens:

| Screen | File | Role |
|--------|------|------|
| `ReaderScreen` | `src/ui/ReaderScreen.h/cpp` | Reading view with bottom bar |
| `BookListScreen` | `src/ui/BookListScreen.h/cpp` | Book selection with search |
| `ChapterGridScreen` | `src/ui/ChapterGridScreen.h/cpp` | Chapter grid for selected book |
| `SettingsScreen` | `src/ui/SettingsScreen.h/cpp` | Full-screen settings |
| `HighlightBrowserScreen` | `src/ui/HighlightBrowserScreen.h/cpp` | Browse highlights by color |
| `CenterMenu` | `src/ui/CenterMenu.h/cpp` | Centered dialog menu |
| `CreditsOverlay` | `src/ui/CreditsOverlay.h/cpp` | Credits overlay |

### ReaderScreen

The root screen. Contains the document rendering (infinite scroll text), highlight rendering, scrollbar, and bottom bar.

**Responsibilities:**
- Draw Bible text (via Renderer)
- Draw highlight rectangles (via Highlighter query)
- Draw scrollbar
- Draw and manage bottom bar (show/hide on scroll direction)
- Handle text input: scroll, highlight selection, context menu, keyboard shortcuts
- Open center menu on book code tap

**Bottom Bar:**

```
[◄]  GEN 1  [►]
```

- `◄` (40px wide): emits NavigateEvent with previous chapter ref
- `GEN 1` (flex): current chapter reference, tappable → opens CenterMenu
- `►` (40px wide): emits NavigateEvent with next chapter ref
- Background: `theme::WINDOW_BG`, 50px height
- Shows on scroll-up (accumulated ≥ 30px), hides on scroll-down (accumulated ≥ 30px)
- Slides up/down with smooth animation (0.2s ease)

**Keyboard shortcuts:**
- `←` / `→`: prev/next chapter (same as bottom bar buttons)
- `G`: open CenterMenu (alternative to tapping book code)
- `S`: push SettingsScreen directly (bypass menu)
- `A`: show CreditsOverlay directly (bypass menu)
- `Escape`: dismiss overlay/menu if active, otherwise no-op (already on root)

### CenterMenu

A centered dialog with backdrop. Shows four options:

| Option | Action |
|--------|--------|
| Books | Push BookListScreen |
| Settings | Push SettingsScreen |
| Highlights | Push HighlightBrowserScreen |
| Credits | Show CreditsOverlay |

Dismissed by: tapping outside, tapping X, pressing Escape.

### BookListScreen

Full-screen scrollable list of 66 books in canonical order.

```
┌──────────────────────────────┐
│  ← Back         Books        │
├──────────────────────────────┤
│  [🔍 Search books...      ]  │
├──────────────────────────────┤
│  Genesis                     │
│  Exodus                      │
│  Leviticus                   │
│  ...                         │
│  Revelation                  │
└──────────────────────────────┘
```

- Search bar filters by prefix match on book code or full name (case-insensitive, max 5 suggestions)
- Reuses auto-complete logic from existing GoToDialog
- Tapping a book pushes ChapterGridScreen for that book

### ChapterGridScreen

Full-screen grid of chapter numbers.

```
┌──────────────────────────────┐
│  ← Back     Genesis          │
├──────────────────────────────┤
│                              │
│  1   2   3   4   5           │
│  6   7   8   9  10           │
│ 11  12  13  14  15           │
│  ...                         │
│ 46  47  48  49  50           │
│                              │
└──────────────────────────────┘
```

- 5-column grid, cells ~60×40px (scaled)
- Tapping a chapter → pops back to Reader, loads chapter via NavigateEvent

### SettingsScreen

Full-screen settings (replaces current modal SettingsPanel).

```
┌──────────────────────────────┐
│  ← Back      Settings        │
├──────────────────────────────┤
│                              │
│  Font:   [−]  24  [+]        │
│  Source: [USFM] [API]        │
│  Color:  ■ ■ ■ ■ ■           │
│                              │
└──────────────────────────────┘
```

Same controls as the current SettingsPanel but full-screen. Source row hidden when no API key. Changes apply immediately.

### HighlightBrowserScreen

Full-screen highlight browser (detailed spec in `03-modules/Highlighting System.md`).

```
┌──────────────────────────────┐
│  ← Back    Highlights        │
├──────────────────────────────┤
│  Color:  ■ ■ ■ ■ ■           │
├──────────────────────────────┤
│  Gen 1:3                     │
│  No princípio criou...       │
│                              │
│  Gen 1:5                     │
│  ...                         │
└──────────────────────────────┘
```

- Color swatch filter at top
- Scrollable list of matching highlights
- Tap item → navigate to verse in Reader

### CreditsOverlay

Same as current About overlay. Centered panel with:
- App name + version
- "Built with Raylib & C++17"
- "Data: USFM (offline) + YouVersion API (online)"
- X close button or tap-outside to dismiss

## InputHandler Updates

The InputHandler must be aware of the navigation stack. When a non-Reader screen is active:
- All keyboard input is routed to the active screen first
- Arrow keys navigate the active screen (e.g., scrolling BookList) rather than scrolling text
- Escape pops the screen (or dismisses overlay/menu)
- Click/touch events are routed to the active screen's HandleInput()

```cpp
class InputHandler {
public:
    InputHandler(EventBus& eventBus, NavigationStack& navStack, ...);

    void Poll(float deltaTime);

private:
    bool HandleShortcuts();      // G, S, A, Escape at root level
    void HandleScroll();         // Mouse wheel, Up/Down (only on root)
    void HandlePressFSM();       // Highlight selection (only on root)
    void HandleWindowResize();   // Always active
    // ...
};
```

## Bottom Bar Show/Hide Logic

```
On scroll velocity change:
  if velocity < 0 and |accumulated| > threshold → show bar (slide up)
  if velocity > 0 and accumulated > threshold → hide bar (slide down)

accumulated is a running sum of scroll deltas since last show/hide toggle
threshold = 30px (configurable)
```

The bar slide animation uses the same `1 - exp(-k * dt)` ease as the smooth scroll, with a fixed speed constant.

## Reusable UI Components

All screens use shared component functions for consistent layout. Each is a free function accepting position, font, style parameters:

| Component | Location | Signature |
|-----------|----------|-----------|
| `DrawHeaderBar` | `src/ui/components.h` | `void DrawHeaderBar(const Font&, float headingSize, const char* title, bool hasBack)` |
| `DrawBottomBar` | `src/ui/components.h` | `void DrawBottomBar(const Font&, float headingSize, const char* ref)` |
| `DrawMenuItem` | `src/ui/components.h` | `bool DrawMenuItem(const Font&, Rectangle rect, const char* label, Vector2 mouse)` |
| `DrawColorSwatches` | `src/ui/components.h` | `int DrawColorSwatches(const Font&, Rectangle rect, const HighlightType* types, int count, int activeId, Vector2 mouse)` |
| `DrawGridCell` | `src/ui/components.h` | `bool DrawGridCell(const Font&, Rectangle cell, const char* label, bool highlighted, Vector2 mouse)` |
| `DrawListItem` | `src/ui/components.h` | `bool DrawListItem(const Font&, Rectangle rect, const char* title, const char* subtitle, Vector2 mouse)` |

## Files (Phase 11 structure)

- `src/ui/NavigationStack.h/cpp` — Screen management
- `src/ui/Screen.h` — Screen interface
- `src/ui/ReaderScreen.h/cpp` — Main reading screen (extracted from current main.cpp/App)
- `src/ui/BookListScreen.h/cpp` — Book selection
- `src/ui/ChapterGridScreen.h/cpp` — Chapter selection
- `src/ui/SettingsScreen.h/cpp` — Full-screen settings
- `src/ui/HighlightBrowserScreen.h/cpp` — Highlight browsing (Phase 13)
- `src/ui/CenterMenu.h/cpp` — Center dialog menu
- `src/ui/CreditsOverlay.h/cpp` — Credits overlay
- `src/ui/components.h/cpp` — Reusable UI component functions
- `src/renderer/Renderer.h/cpp` — Unchanged (still draws text + highlights)
- `src/input/InputHandler.h/cpp` — Updated to route through NavigationStack
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

**Implemented (Phase 9 Sprint 4):** Each heading type has its own distinct style:

| Type | Font | Scale | Color | Alignment |
|------|------|-------|-------|-----------|
| **BookTitle** | `headingFont` | 1.6× | `BLACK` | Centered |
| **SectionHeading** | `headingFont` | 1.3× | `DARKGRAY` | Centered |
| **ChapterLabel** | `headingFont` | 1.6× | `(80, 80, 80)` | Centered |

The `case` in `Renderer::drawSpan()` is split into three separate branches. No layout engine changes needed — all three are already positioned as centered headings by the LayoutEngine.

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
- `getContentTop()` returns the top bar height (60px × DPI scale) used by Renderer for document offset
- Context menu on long-press/right-click: single-row popup with "Del" button (red) + 5 pastel color swatches (side-by-side)
- Context menu click handling: delete highlight, recolor highlight, or dismiss on outside/Escape
- Context menu overflow: flips to left of cursor when it would overhang right edge
- Go-to dialog: text input field with auto-complete (matches book code or full name, case-insensitive, up to 5 suggestions), keyboard navigation (Enter loads chapter, Tab auto-completes selected, Up/Down cycle, Backspace deletes, Escape dismisses), error feedback (red border on invalid chapter)
- Settings panel: modal overlay with font size A–/A+ buttons (12–36 range, grayed at limits), USFM/Online version toggle (hidden when no CompositeProvider), active color swatch selector with black border on selected, close button (X)
- About overlay: toggled with 'A', shows app name, Raylib credit, data sources, keyboard shortcuts
- Settings apply: font size → `LayoutEngine::setFontSize` + `invalidateCache` + `Renderer::setFontSize` + `DocumentManager::invalidateLayouts` + persist; version toggle → `CompositeProvider::setPrimary` + reload current chapter + persist; color swatch → `Highlighter::setActiveTypeId` + persist
- Splash screen: text-only "TheWord" at 48pt + "Loading..." at 20pt drawn before font loading (handled in main.cpp, before UIManager creation)
- DPI scaling: all positions, sizes, and fonts multiplied by `scale` factor on Android

### Interface
```cpp
class UIManager {
public:
    UIManager(Highlighter& highlighter);
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
| `A` | Any (no dialog active) | Toggle about/credits overlay |
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
