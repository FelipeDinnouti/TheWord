# Highlighting System

> Status: Planned (Phase 7) | Last Updated: 2026-06-22

## Overview

The highlighting system is the core motivation for the entire project. Users can select words or ranges of words and assign them highlight colors. Highlights are stored as word ID ranges, making them resolution-independent.

## Specification

### Data Structures

```cpp
struct Highlight {
    int id;
    int startWord;
    int endWord;
    int typeId;
};

struct HighlightType {
    int id;
    std::string name;
    Color color;
};
```

### MVP: One Color

The MVP uses a single highlight type (yellow). Multiple colors will be added later.

### Hit Detection

1. Screen position is converted to document space using the current scroll position
2. The layout engine finds which span contains the Y position (binary search by line Y)
3. Within the matching line, spans are checked left-to-right for X containment
4. The matching span's word range identifies the clicked word

### Drag Selection

1. On mouse press: record the word under cursor as `selectionStart`
2. On mouse move (while held): update `selectionEnd` to current word
3. On mouse release: create a `Highlight` covering `selectionStart` → `selectionEnd`

### Rendering

1. For each visible span, query the highlighter for highlights covering the span's word range
2. Draw filled rectangles behind the text using the highlight color
3. Draw the text on top

### Word ID Source

Word IDs come from `ChapterData::words[]`. Each word has a globally unique ID assigned by the ChapterProvider. IDs are stable across sessions as long as the same Bible version is used.

## Persistence

Highlights will be saved to SQLite (see Phase 8). Word ID ranges are stable across sessions and re-layouts.

## Design Constraint: Coupling with Persistence (Phase 8)

The highlighter and the persistence manager share the same core data (`Highlight` / `HighlightType`). **Do not implement one without considering the other.** See `02-architecture/Architecture Overview.md` → "Cross-Cutting Concerns → 1. Highlight + Persistence" for the full discussion.

To keep the highlighter testable without SQLite, define a pure virtual `PersistenceInterface` that the highlighter calls:

```cpp
class PersistenceInterface {
public:
    virtual ~PersistenceInterface() = default;
    virtual std::vector<Highlight> loadHighlights() = 0;
    virtual void saveHighlight(const Highlight& h) = 0;
    virtual void removeHighlight(int id) = 0;
};
```

The highlighter takes a `PersistenceInterface&` in its constructor. During Phase 7, pass an in-memory stub. When Phase 8 arrives, pass the real `PersistenceManager` without changing any highlight logic.

## Files

- `src/highlight/Highlighter.h/cpp`
- `src/highlight/HighlightType.h/cpp`
- `src/highlight/PersistenceInterface.h` (shared contract)
