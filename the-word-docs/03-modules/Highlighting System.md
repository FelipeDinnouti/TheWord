# Highlighting System

> Status: Updated for Phase 13 (in progress) | Last Updated: 2026-06-29

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
    std::string providerName;  // Added in Phase 9 Sprint 3
    // Phase 13 additions:
    std::string bookId;        // e.g. "GEN"
    int chapterNum;            // e.g. 1
    int verseStart;            // First verse in the highlighted range
    int verseEnd;              // Last verse in the highlighted range
};

struct HighlightType {
    int id;
    std::string name;
    Color color;
};
```

The `bookId`, `chapterNum`, `verseStart`, and `verseEnd` fields allow the Highlight Browser to display verse references without needing to load the original chapter to resolve word IDs. These are populated at highlight creation time (in `Highlighter::EndSelection()`) from the `ChapterData` being viewed.

### Colors

5 pastel colors: Yellow, Pink, Green, Blue, Orange. Seeded in the database on first run.
Active color is selectable via settings panel and persisted as a preference.

### Hit Detection

1. Screen position is converted to document space using the current scroll position
2. The layout engine finds which span contains the Y position (binary search by line Y)
3. Within the matching line, spans are checked left-to-right for X containment
4. The matching span's word range identifies the clicked word

### Drag Selection

1. On mouse press: record the word under cursor as `selectionStart`
2. On mouse move (while held): update `selectionEnd` to current word
3. On mouse release: create a `Highlight` covering `selectionStart` → `selectionEnd`
4. Populate `bookId`, `chapterNum`, `verseStart`, `verseEnd` from the currently loaded chapter

### Rendering

1. For each visible span, query the highlighter for highlights covering the span's word range
2. Draw filled rectangles behind the text using the highlight color
3. Draw the text on top

### Word ID Source

Word IDs come from `ChapterData::words[]`. Each word has a globally unique ID assigned by the ChapterProvider. IDs are stable across sessions as long as the same Bible version is used.

## Highlight Browser (Phase 13)

The Highlight Browser allows users to find and navigate to highlights by color. It is a full-screen pushed onto the navigation stack, accessed from the center menu.

### UI Layout

```
┌──────────────────────────────┐
│  ← Back    Highlights        │  header bar
├──────────────────────────────┤
│  Color:  ■ ■ ■ ■ ■           │  filter swatches
├──────────────────────────────┤
│  Gen 1:3                     │  reference title
│  No princípio criou Deus...  │  verse text
│                              │
│  Gen 1:5                     │
│  E chamou Deus à luz...      │
│                              │
│  (scrollable list)           │
└──────────────────────────────┘
```

### Interaction Flow

1. User taps "Highlights" in the center menu
2. HighlightBrowserScreen pushes onto the navigation stack
3. By default, no color is selected (prompt: "Select a color to browse")
4. User taps a color swatch → list populates with all highlights of that color
5. Each list item shows:
   - Reference line: `Book Chapter:Verse` (e.g., "Gen 1:3")
   - Verse text line: the words from the highlighted range, truncated with `...` if > 80 chars
   - The highlighted words have the highlight color drawn behind them
6. User taps a list item → pops back to Reader, loads the chapter, scrolls to make the verse visible at the top of the viewport

### Highlighter API Additions

```cpp
class Highlighter {
    // Existing methods...

    // Phase 13 additions:
    std::vector<const Highlight*> GetHighlightsByType(int typeId) const;
    std::vector<const Highlight*> GetHighlightsByType(int typeId, int limit, int offset) const;
    int GetHighlightCountByType(int typeId) const;

    // Helper to update EndSelection to capture chapter context:
    void EndSelection(const std::string& bookId, int chapterNum);
};
```

### Reference Field Population

When a highlight is created in `EndSelection()`:

```cpp
void Highlighter::EndSelection(const std::string& bookId, int chapterNum) {
    if (!selecting) return;
    selecting = false;

    int start = std::min(selectionStart, selectionEnd);
    int end = std::max(selectionStart, selectionEnd);
    if (start < 0) return;

    // Determine verse range from word IDs — needs a lookup mechanism
    int vStart = resolveVerseForWord(start);
    int vEnd = resolveVerseForWord(end);

    Highlight h;
    h.id = nextId++;
    h.startWord = start;
    h.endWord = end;
    h.typeId = activeTypeId;
    h.providerName = currentProvider;
    h.bookId = bookId;
    h.chapterNum = chapterNum;
    h.verseStart = vStart;
    h.verseEnd = vEnd;

    highlights.push_back(h);
    persistence.SaveHighlight(h);
}
```

**Verse resolution**: Since `ChapterData::words[]` has each word's `verseId`, and the highlight is created while the chapter is loaded, the verse range can be determined by scanning the words array. The DocumentManager provides the current chapter's data to the Highlighter via an event or direct call.

### Navigation to Verse

When a user taps a highlight in the browser:
1. The screen pops back to the Reader
2. A `NavigateToHighlightEvent{navigateRef, wordId}` is emitted
3. Reader loads the chapter, then searches for the line containing `wordId`
4. Scrolls so that line is at the top of the viewport (minus content padding)

```cpp
struct NavigateToHighlightEvent {
    std::string chapterRef;  // e.g. "GEN.3"
    int wordId;              // Target word to scroll into view
};
```

The Reader handles this by:
1. `LoadInitialChapter(chapterRef)` → resets scroll to 0
2. Searching the loaded layout for `wordId` in any span's word range
3. Setting `targetScrollY` to position that line at `contentTop`

### Persistence Schema Migration

New columns added to the `highlights` table:

```sql
ALTER TABLE highlights ADD COLUMN book_id TEXT;
ALTER TABLE highlights ADD COLUMN chapter_num INTEGER;
ALTER TABLE highlights ADD COLUMN verse_start INTEGER;
ALTER TABLE highlights ADD COLUMN verse_end INTEGER;
```

Existing highlights get `NULL` for these fields. The Highlight Browser can either:
- Show them with `"Unknown"` as the reference
- Backfill them lazily when first viewed (load the chapter and resolve)

Lazy backfill is recommended to avoid a one-time migration of potentially thousands of highlights.

### Empty State

When no highlights of the selected color exist:

```
  ┌──────────────────────┐
  │                      │
  │  No highlights of    │
  │  this color.         │
  │                      │
  │  Select a different  │
  │  color or create new │
  │  highlights in the   │
  │  Reader.             │
  │                      │
  └──────────────────────┘
```

## Persistence

Highlights are saved to SQLite (see Phase 8). Word ID ranges are stable across sessions and re-layouts. The new reference fields (bookId, chapterNum, verseStart, verseEnd) are persisted alongside the existing word ID fields.

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
- `src/highlight/HighlightType.h/cpp` (phase 7 — may be merged into PersistenceInterface.h)
- `src/highlight/PersistenceInterface.h` (shared contract)
- `src/ui/HighlightBrowserScreen.h/cpp` (Phase 13 — new screen)
- `src/event/Events.h` — Add `NavigateToHighlightEvent`
