# Document Manager

> Status: Updated 2026-06-22

Files: `src/document/DocumentManager.h/cpp`

## Overview

The Document Manager owns the infinite scroll mechanism. It talks to a `ChapterProvider` (which could be `USFMParser` or `BibleClient`) to load chapter data, maintains a sliding window of loaded chapters, tracks their positions in document space, handles prepend/append of chapters as the user scrolls, and provides smooth scrolling with lerp-based interpolation.

## Interface

```cpp
class DocumentManager {
public:
    DocumentManager(LayoutEngine& layoutEngine, float viewportHeight,
                    ChapterProvider& primaryProvider,
                    ChapterProvider* fallbackProvider = nullptr);

    void loadInitialChapter(const std::string& book, int chapter);
    void loadInitialChapter(const std::string& chapterId); // "JHN.3" format

    void update(float deltaTime);
    void scrollBy(float delta);

    float getScrollY() const;
    float getTotalHeight() const;
    float getViewportHeight() const;

    void setViewportHeight(float height);
    void invalidateLayouts();

    bool canPrepend() const;
    bool canAppend() const;

    void getVisibleSpans(std::vector<std::pair<Span, float>>& docSpans) const;

    const std::string& getCurrentChapterId() const;
    std::string getChapterTitle() const;

private:
    ChapterProvider& primaryProvider;
    ChapterProvider* fallbackProvider; // optional secondary
    // ...
};
```

## Responsibilities

### Chapter Loading
- `loadInitialChapter()` — Clears all chapters, calls `ChapterProvider::LoadChapter()` for the starting chapter
- If primary provider returns `nullopt`, falls back to `fallbackProvider`
- Stores the full `ChapterData` for later re-layout

### Chapter Lifecycle
- Chapters are loaded dynamically as the user scrolls near boundaries
- `canPrepend()` / `canAppend()` check if there are previous/next chapters
- Prev/next chapter navigation uses `BibleBooks.h` (`src/core/BibleBooks.h`)

### Scroll Management
- Scroll position is stored in **document space** (not screen space).
- Smooth scrolling via lerp: `scrollY += (targetScrollY - scrollY) * SMOOTH_SPEED * dt`
- Scroll is clamped to `[0, totalHeight - viewportHeight]`.

### Visible Spans
- `getVisibleSpans()` returns spans in **document space** (not screen space).
- The renderer converts to screen space: `screenY = docY - scrollY + contentTop`.
- Only spans within `[scrollY - viewportHeight, scrollY + viewportHeight * 2]` are returned.

### Chapter Navigation
- Uses `BibleBooks.h` — data-driven table of all 66 books with chapter counts.
- `FindBookIndex(code)` — O(n) lookup by book abbreviation.
- `GetPreviousChapter(ref)` / `GetNextChapter(ref)` — Navigate within and across books.
- `ParseChapterRef(ref, book, chapter)` — Split "JHN.3" into book="JHN", chapter=3.
- `ChapterIdToTitle(id)` — Convert "JHN.3" to "John 3".

### Layout Invalidation
- `invalidateLayouts()` clears the LayoutEngine cache, then re-layouts all loaded chapters using their stored `ChapterData`.
- Must be called after font size change or window resize.

## LoadedChapter

```cpp
struct LoadedChapter {
    std::string chapterId;    // e.g., "JHN.3"
    ChapterData data;         // Raw data for re-layout (segments + words)
    ChapterLayout layout;     // Cached layout
    float startY;             // Document-space start Y
    float height;             // Chapter height in document space
};
```

The `data` field replaces the old `rawText`. It stores the full `ChapterData` so that `invalidateLayouts()` can re-layout chapters without re-fetching from the provider.

## Source Selection

At construction, the DocumentManager receives:
- `primaryProvider` — The preferred data source (e.g., `BibleClient` for online API)
- `fallbackProvider` — An optional secondary source (e.g., `USFMParser` for offline fallback)

If `primaryProvider.LoadChapter()` returns `nullopt`, the manager tries `fallbackProvider`. If both return `nullopt`, the chapter is skipped with a logged warning.

## Rendering Integration

The Renderer queries `getVisibleSpans()` to get the spans to draw. Each span carries its document-space position and word range. The Renderer:
1. Converts document Y to screen Y
2. Draws the text at the screen position
3. (Future) Queries the Highlighter for overlapping highlights and draws background rectangles

## Key Design Points

1. **Provider-agnostic**: DocumentManager never knows which ChapterProvider is active
2. **Rich text preserved**: ChapterData carries segments and words — re-layout keeps full structure
3. **Graceful fallback**: If primary source fails, secondary source is tried
4. **Anchor-fixed prepend**: scroll position adjusts when prepending to keep visible text stable
