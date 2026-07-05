# UI Philosophy

> Status: Active Design | Last Updated: 2026-06-26

## Core Principle

TheWord is a **reading-first** app. Every UI decision must preserve the immersion of reading continuous Bible text. Navigation, settings, and browsing are secondary — they should be accessible but never intrusive.

## Navigation Model: Navigation Stack + Bottom Bar

The app uses a **navigation stack** (push/pop screens) with a persistent **bottom bar** on the reading screen. There is no tab bar.

```
Reader (root screen)
  ├── tap book code → Center Menu (centered dialog)
  │     ├── Books → BookList → ChapterGrid → Reader (at new chapter)
  │     ├── Settings → Settings → Reader
  │     ├── Highlights → HighlightBrowser → Reader
  │     └── Credits → Credits overlay → Reader
  └── ◄/► or ←/→ keys → Reader (at prev/next chapter)
```

### Screen Transitions

- **Push**: New screen slides in from right (or fades in for overlays)
- **Pop**: Current screen slides out to right, revealing previous
- **Overlay**: Center menu and credits appear as centered dialogs over a backdrop

### Back Navigation

Every pushed screen has three ways to go back:
1. **← Back button** at top-left of every non-root screen
2. **Swipe-right gesture** from left edge (mobile)
3. **Escape key** (desktop)

## Bottom Bar

The reading screen's only persistent UI element.

### Layout

```
┌──────────────────────────────┐
│  ◄      GEN 1          ►    │  50px tall
└──────────────────────────────┘
   40px       flex       40px
```

- `◄` (left button, 40px wide): navigate to previous chapter
- `GEN 1` (center, flex): current book code + chapter number, clickable → center menu
- `►` (right button, 40px wide): navigate to next chapter
- Background: solid `theme::WINDOW_BG`, no border/shadow

### Show/Hide Behavior

- **Show**: When user scrolls up (negative scroll velocity) by at least 30px accumulated
- **Hide**: When user scrolls down (positive scroll velocity) by at least 30px accumulated
- **Animation**: Smooth slide-up (show) / slide-down (hide), 0.2s ease
- **Always visible on first load**: Bottom bar is shown when the app starts, hides on first scroll-down
- **No delay**: Bar shows/hides immediately after threshold is crossed (no timer)

### Why Overlay?

The bar overlays content with a solid background rather than pushing text up because:
1. Avoids layout reflow on every show/hide
2. Content underneath is briefly obscured during scroll, which is acceptable
3. Simpler implementation — no coordinate system adjustment

## Center Menu

Opened by tapping the book code in the bottom bar. A centered dialog with rounded corners (optional), solid background, and a list of options.

```
┌──────────────────────┐
│      Menu            │
│                      │
│  Books               │  → pushes BookList screen
│  Settings            │  → pushes Settings screen
│  Highlights          │  → pushes HighlightBrowser screen
│  Credits             │  → shows credits overlay
│                      │
│  [Close]             │  X button top-right
└──────────────────────┘
```

- Dismissed by: tapping outside, pressing Escape, tapping X
- Each option is a tappable row with icon (optional) + label
- On dismiss: bottom bar reappears if it was hidden
- The backdrop is semi-transparent black (`theme::OVERLAY_BG`)

## Top Bar

**Removed.** The chapter reference previously shown at the top-left is now in the bottom bar center. Removing the top bar gives more vertical space for text content.

## Screen Specifications

### BookList Screen

A full-screen scrollable list of all 66 books in **Canonical order** (Genesis → Revelation).

```
┌──────────────────────────────┐
│  ← Back         Books        │  ← header bar
├──────────────────────────────┤
│  [🔍 Search books...      ]  │  ← search input
├──────────────────────────────┤
│  Genesis                     │  ← tappable row
│  Exodus                      │
│  Leviticus                   │
│  ...                         │
│  Revelation                  │
└──────────────────────────────┘
```

- **Search bar**: at top, below header. Filters books by prefix match on code or full name (case-insensitive). Same matching logic as existing GoToDialog.
- No section headers (canonical order is well-known)
- Tapping a book → pushes **ChapterGrid** screen for that book
- Search auto-focuses when screen appears

### ChapterGrid Screen

A full-screen grid of chapter numbers for the selected book.

```
┌──────────────────────────────┐
│  ← Back     Genesis          │  ← header bar with book name
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

- Grid: 5 columns, each cell ~60x40px with rounded rect + number
- Shows all chapters from 1 to `book.chapterCount`
- Tapping a chapter → pops back to Reader, loads that chapter
- Current chapter (if viewing this book) could be highlighted

### Settings Screen

A full-screen replacement for the current modal settings panel.

```
┌──────────────────────────────┐
│  ← Back      Settings        │  ← header bar
├──────────────────────────────┤
│                              │
│  Font:   [−]  24  [+]        │
│                              │
│  Source: [USFM] [API]        │
│                              │
│  Color:  ■ ■ ■ ■ ■           │
│                              │
└──────────────────────────────┘
```

- Same controls as current settings panel (font size, source toggle, color swatches)
- Full-screen with back button
- Changes apply immediately
- Source row hidden when no API key

### HighlightBrowser Screen

A full-screen list of all highlights filtered by color.

```
┌──────────────────────────────┐
│  ← Back    Highlights        │  ← header bar
├──────────────────────────────┤
│                              │
│  Color:  ■ ■ ■ ■ ■           │  ← filter swatches
│                              │
│  ┌────────────────────────┐  │
│  │ Gen 1:3                │  │  ← reference (title)
│  │ No princípio criou...  │  │  ← verse text (body, truncated)
│  ├────────────────────────┤  │
│  │ Gen 1:5                │  │
│  │ ...                    │  │
│  └────────────────────────┘  │
│                              │
└──────────────────────────────┘
```

- **Color filter**: row of 5 color swatches at top. Tap to filter. Active color has a border.
- **List**: scrollable list of all highlights matching the selected color
- **Each item**:
  - Reference line: `Book Chapter:Verse` using the highlight's stored reference
  - Verse text line: the actual words from the highlighted range, ended with `...` if truncated
  - Background highlight color applied to the text area
- **Tapping an item**: pops back to Reader, loads the chapter, scrolls to make the verse visible at the top of the viewport
- **Empty state**: "No highlights found" centered text when no matches
- **No selection** (no color selected): shows all highlights across all colors, or shows nothing with a prompt to select a color

### Credits Overlay

Small centered overlay (same as current About dialog) showing app name, version, Raylib credit, data sources. Dismissed by tapping outside or pressing Escape.

## Verse Number Identifiers

- Rendered at layout time (Span type: `VerseNumber`)
- Positioned as superscript: smaller font (0.65×), Y offset of -4px
- Format: `¹.` (digit + dot, using the verse number digit)
- Color: grey (`DOC_VERSE_NUMBER`)
- First verse of each chapter is always numbered
- The verse number width is accounted for in the layout engine (affects word wrapping)

## Reusable Components

UI code should be built from composable, parameterized functions/structs rather than duplicated inline draw calls. Identified reusable patterns:

| Component | Parameters | Used In |
|-----------|-----------|---------|
| `DrawHeaderBar(text, hasBack)` | Title, back button visibility | All pushed screens |
| `DrawBottomBar(prev, ref, next)` | Prev click, center click, next click | Reader only |
| `DrawMenuDialog(items[], title)` | Menu item list, title | Center menu |
| `DrawColorSwatches(active, types, onClick)` | Active ID, type list, click handler | Settings, HighlightBrowser |
| `DrawTextInput(placeholder, text, error)` | Ghost text, current value, error flag | BookList search |
| `DrawGridButton(number, x, y, w, h)` | Label, position, size, clickable | ChapterGrid |
| `DrawListItem(label, subtitle, x, y, w, h)` | Primary text, secondary text, clickable | BookList, HighlightBrowser |

Each component is a free function or a small struct with a `Draw()` method, no base classes or inheritance.

## Why Not a UI Library (Clay)?

[Clay](https://github.com/nicbarker/clay) is a single-header C library that does flexbox-style layout with pointer events. For this project's scope (~20 distinct UI elements across ~6 screens), the overhead of integrating and debugging an external layout engine outweighs the benefit. The existing `DrawRectangle` + `MeasureTextEx` approach is more predictable for text-heavy layouts and keeps the dependency graph minimal. If the UI grows significantly (50+ elements), revisiting this decision is reasonable.

## Design Constraints

1. **No overlapping screens**: Only one screen is visible at a time (except overlays)
2. **Fast transitions**: Screen pushes/pops should be instant or < 0.2s
3. **No animation on text content**: Only UI elements animate (bottom bar slide, screen transitions)
4. **Keyboard shortcuts always work**: `G` (go-to, even though bottom bar exists), `S` (settings), `A` (about), `Escape` (back/dismiss), `←`/`→` (prev/next chapter)
5. **Mobile-first**: Touch targets minimum 40x40px, bottom bar is thumb-reachable
