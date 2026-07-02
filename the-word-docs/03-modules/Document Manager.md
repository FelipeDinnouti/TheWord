# Document Manager

> Status: Updated 2026-07-01

Files: `src/document/DocumentManager.h/cpp`

## Overview

The Document Manager owns the infinite scroll mechanism. It talks to a `ChapterProvider` (which could be `USFMParser` or `BibleClient`) to load chapter data, maintains a sliding window of loaded chapters, tracks their positions in document space, handles prepend/append of chapters as the user scrolls, and provides smooth scrolling with lerp-based interpolation.

Chapter navigation via the center menu is **asynchronous**: selecting a chapter in the grid pops back to the reader immediately and loads the chapter in a background thread (via `std::async`). When the load completes, a `ChapterLoadedEvent` is emitted so the highlighter context can be updated.

## Interface

```cpp
class DocumentManager {
public:
    DocumentManager(EventBus& eventBus, LayoutEngine& layoutEngine,
                    float viewportHeight, ChapterProvider& primaryProvider,
                    float contentTop = 60.0f);

    void LoadInitialChapter(const std::string& chapterId);      // async (runtime navigation)
    void LoadInitialChapterSync(const std::string& chapterId);   // sync (startup only)

    void Update(float deltaTime);
    bool HasPendingLoads() const;
    bool HasMomentum() const;

    float GetScrollY() const;
    float GetTotalHeight() const;
    float GetViewportHeight() const;

    void SetViewportHeight(float height);
    void ScrollTo(float y);
    void InvalidateLayouts();

    void GetVisibleSpans(std::vector<std::pair<Span, float>>& docSpans) const;
    int HitTestWord(float screenX, float screenY, float scrollY) const;

    const std::string& GetCurrentChapterId() const;
    std::string GetChapterTitle() const;
    const ChapterLayout* GetCurrentLayout() const;
    const ChapterData* GetCurrentChapterData() const;

private:
    EventBus& eventBus_;
    LayoutEngine& layoutEngine;
    ChapterProvider& primaryProvider;
    // ...
};
```

## Responsibilities

### Chapter Loading — Two Paths

The manager provides two loading methods for different contexts:

| Method | Use | Behavior |
|--------|-----|----------|
| `LoadInitialChapter()` | Runtime navigation | Async — clears chapters immediately, launches `std::async` to load via provider. Completion is checked in `Update()` and emits `ChapterLoadedEvent`. |
| `LoadInitialChapterSync()` | Startup only | Sync — clears chapters, blocks on I/O + layout, inserts result directly. Called once from `App::Init()` behind the splash screen. |

Both clear the current chapter state and move any in-flight async loads to the graveyard.

### Async Loading Flow

```
User selects chapter in grid
  → ChapterGridScreen::HandleInput()
    → navStack_.PopAll()              ← returns to reader immediately
    → eventBus_.Emit(NavigateEvent)
      → App handler calls LoadInitialChapter(ref)
        → clears chapters, stores future in initialLoadFuture_
        → returns immediately

Next frame:
  → DocumentManager::Update()
    → checks initialLoadFuture_.wait_for(0)
    → not ready → skip, reader draws blank

... future completes on background thread ...

Next frame:
  → DocumentManager::Update()
    → initialLoadFuture_ is ready
    → future.get() → LoadedChapter
    → chapters.clear()
    → insert result, reset scroll
    → eventBus_.Emit(ChapterLoadedEvent{chapterId})
      → App handler calls highlighter_->SetChapterContext(...)
    → Reader draws the new chapter
```

### Pending Load Tracking

- `HasPendingLoads()` — returns `true` if `initialLoadFuture_` is active OR any `pendingLoads_` (adjacent preloads) are still in-flight
- The main loop uses this to prevent GPU idle-skipping during loads
- `HasMomentum()` — returns `true` while smooth-scroll animation is active

### Adjacent Chapter Preloading (Scroll)

As the user scrolls near loaded chapter boundaries, `Update()` calls `TryLoadAdjacent()` to asynchronously preload the next/previous chapter via `std::async`. Completed loads are inserted into the `chapters` vector on the next `Update()` tick. This is the same mechanism as the initial load but uses a separate `pendingLoads_` queue.

### Scroll Management
- Scroll position is stored in **document space** (not screen space).
- Smooth scrolling via lerp: `scrollY += (targetScrollY - scrollY) * SMOOTH_SPEED * dt`
- Scroll is clamped to `[0, totalHeight - viewportHeight]`.

### Visible Spans
- `GetVisibleSpans()` returns spans in **document space** (not screen space).
- The renderer converts to screen space: `screenY = docY - scrollY + contentTop`.
- Only spans within `[scrollY - viewportHeight, scrollY + viewportHeight * 2]` are returned.

### Chapter Navigation
- Uses `BibleBooks.h` — data-driven table of all 66 books with chapter counts.
- `FindBookIndex(code)` — O(n) lookup by book abbreviation.
- `GetPreviousChapter(ref)` / `GetNextChapter(ref)` — Navigate within and across books.
- `ParseChapterRef(ref, book, chapter)` — Split "JHN.3" into book="JHN", chapter=3.
- `ChapterIdToTitle(id)` — Convert "JHN.3" to "John 3".

### Layout Invalidation
- `InvalidateLayouts()` moves any in-flight `initialLoadFuture_` and `pendingLoads_` to the graveyard, clears the LayoutEngine cache, then re-layouts all loaded chapters using their stored `ChapterData`.
- Must be called after font size change or window resize.

### Future Graveyard

Because `std::future`'s destructor blocks until the async task completes, cancelled futures are moved to `pendingGraveyard_`. On each `Update()`, the graveyard is drained of already-completed futures (non-blocking `wait_for(0)` check). This prevents frame drops when cancelling in-flight loads.

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

The `data` field stores the full `ChapterData` so that `InvalidateLayouts()` can re-layout chapters without re-fetching from the provider.

## Source Selection

At construction, the DocumentManager receives a single `primaryProvider` (the `CompositeProvider` when online, `USFMParser` when offline-only). Fallback is handled by the provider chain, not by DocumentManager itself.

## Rendering Integration

The Renderer queries `GetVisibleSpans()` to get the spans to draw. Each span carries its document-space position and word range. The Renderer:
1. Converts document Y to screen Y
2. Draws the text at the screen position
3. Queries the Highlighter for overlapping highlights and draws background rectangles

## Key Design Points

1. **Provider-agnostic**: DocumentManager never knows which ChapterProvider is active
2. **Rich text preserved**: ChapterData carries segments and words — re-layout keeps full structure
3. **Non-blocking navigation**: Chapter switching pops back to reader instantly and loads async
4. **Anchor-fixed prepend**: scroll position adjusts when prepending to keep visible text stable
