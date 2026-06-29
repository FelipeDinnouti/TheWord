# Async Scroll Overhaul — Implementation Plan

> **Status**: Ready — Build Phase | **Target**: Scroll smoothness + async chapter loading

## Problem

Three scroll issues compound into a visible stutter when auto-loading chapters:

1. **Prepend snap** — `scrollY = targetScrollY` in `PrependChapter` kills the in-progress
   lerp, causing a sudden coordinate jump (doc: `DocumentManager.cpp:259`)

2. **Inertial velocity carryover** — `InputHandler`'s `scrollVelocity` accumulates while
   clamped at the document boundary. When `AppendChapter` increases `maxScroll`, the
   pent-up velocity bursts `targetScrollY` forward, lurching the view into new content.

3. **Synchronous loading stutters the frame** — `LayoutChapter` calls `MeasureTextEx`
   for every word (750+ calls per chapter, 5-15ms total). This blocks the main thread
   inside `Update()`, causing a visible frame hitch.

4. **Static margin** — `AUTO_LOAD_MARGIN = 50.0f` doesn't adapt to load latency or
   scroll speed. Prepend has no margin at all (`scrollY <= 0.0f`).

## Strategy

- Decouple all chapter loading + layout onto a worker thread. The main thread only
  inserts completed `LoadedChapter` objects into the chapter list (~microseconds).
- Use an event (`ScrollStopEvent`) to reset residual scroll velocity after loading,
  keeping the convention of event-driven communication between modules.
- Dynamically compute the load margin from measured load times and scroll speed,
  replacing the hard-coded 50px constant.

## Architecture — Async Loading

```
Worker thread:
  ┌──────────────────────────────────────────┐
  │ lock_guard(providerMutex_)                │
  │ data = provider.LoadChapter(book, chap)   │  ← file I/O or network
  │ unlock                                    │
  │                                           │
  │ layout = LayoutChapter(id, data, true)    │  ← MeasureTextEx (CPU, no GPU)
  │    └ skipCache=true (thread-safe mode)    │
  │ if (gen != fontGeneration_) discard       │
  │ return LoadedChapter{id, data, layout, …} │
  └──────────┬───────────────────────────────┘
             │ std::future ready
             ▼
Main thread (Update):
  ┌──────────────────────────────────────────┐
  │ for each pending future:                  │
  │   if ready:                               │
  │     lc = future.get()                     │
  │     calc positions, adjust scroll         │
  │     chapters.insert(...)                  │
  │     emit ScrollStopEvent                  │
  └──────────────────────────────────────────┘
  Then: check if new loads needed (dynamic margin)
```

### Thread safety

| Concern | Mitigation |
|---------|-----------|
| `ChapterProvider::LoadChapter()` | `std::mutex providerMutex_` — locked by worker, also locked in `LoadInitialChapter` |
| `LayoutEngine::LayoutChapter()` | `skipCache=true` parameter — skips write to `cachedLayouts` vector |
| Font data (glyph heap) | `std::atomic<int> fontGeneration_` on DocumentManager + `int layoutGeneration_` on LayoutEngine. Worker checks generation after layout; discard on mismatch |
| `LoadedChapter` insertion | No concurrent writes — worker returns by future, main thread inserts |

## Steps

### Step 1 — Fix PrependChapter scroll snap + add prepend margin

**File**: `src/document/DocumentManager.cpp`

- **Line 259**: Remove `scrollY = targetScrollY;` — let the lerp handle scrollY naturally
- **Line 110**: Change `scrollY <= 0.0f` → `scrollY <= autoLoadMargin_` (use dynamic margin, initialized to `MIN_LOAD_MARGIN`)

**Verification**: Build + run, scroll to top of Genesis — no snap, previous chapter loads smoothly.

---

### Step 2 — Add `ScrollStopEvent` + wire velocity reset

**Files**: `src/event/Events.h`, `src/document/DocumentManager.cpp`, `src/input/InputHandler.cpp`

**`Events.h`** — add:
```cpp
struct ScrollStopEvent {};
```

**`DocumentManager::Update()`** — after any successful `TryLoadAdjacent`:
```cpp
if (loaded) {
    eventBus_.Emit(ScrollStopEvent{});
    targetScrollY = scrollY;
}
```

**`InputHandler` constructor** — subscribe:
```cpp
eventBus_.On<ScrollStopEvent>([this](const auto&) { scrollVelocity = 0.0f; });
```

**Note**: `InputHandler` already receives `EventBus&` in its constructor.

**Verification**: Scroll to bottom with inertia — no lurch into newly appended content.

---

### Step 3 — Dynamic margin tracking

**File**: `src/document/DocumentManager.h` + `.cpp`

Replace `static constexpr float AUTO_LOAD_MARGIN = 50.0f;` with:

```cpp
float autoLoadMargin_ = 50.0f;
float avgScrollSpeed_ = 100.0f;  // EMA, initial estimate
float recentLoadTimes_[5] = {};   // ring buffer
int loadTimeIndex_ = 0;
static constexpr float MIN_LOAD_MARGIN = 30.0f;
static constexpr float MAX_LOAD_MARGIN = 300.0f;
```

**`OnScroll()`** — track scroll speed:
```cpp
float absDelta = std::abs(e.delta);
avgScrollSpeed_ = avgScrollSpeed_ * 0.9f + absDelta * 0.1f;
```

**New method `UpdateLoadTime(float ms)`**:
```cpp
void DocumentManager::UpdateLoadTime(float ms) {
    recentLoadTimes_[loadTimeIndex_ % 5] = ms;
    loadTimeIndex_++;
    int count = std::min(loadTimeIndex_, 5);
    float sum = 0;
    for (int i = 0; i < count; i++) sum += recentLoadTimes_[i];
    float margin = (sum / count) / 1000.0f * avgScrollSpeed_ * 1.5f;
    autoLoadMargin_ = std::clamp(margin, MIN_LOAD_MARGIN, MAX_LOAD_MARGIN);
}
```

Both prepend and append triggers now use `autoLoadMargin_`.

**Verification**: Load a few chapters, verify margin adapts. Slow scroll → smaller margin. Fast scroll → larger. Print to log for observation.

---

### Step 4 — Async loading (full offload)

**File**: `src/document/DocumentManager.h` + `.cpp`, `src/text/LayoutEngine.h` + `.cpp`

#### 4a — LayoutEngine: thread-safe mode + generation counter

**`LayoutEngine.h`** — add:
```cpp
ChapterLayout LayoutChapter(const std::string& chapterId, const ChapterData& data, bool skipCache = false);
int GetGeneration() const { return layoutGeneration_; }
// ...
private:
std::atomic<int> layoutGeneration_{0};
```

**`LayoutEngine.cpp`** — guard cache write:
```cpp
if (existing != cachedLayouts.end()) return *existing;
// ... layout ...
if (!skipCache) cachedLayouts.push_back(layout);
```

Increment `layoutGeneration_` in `InvalidateCache()`, `SetFontSizes()`, and `OnResize()`.

#### 4b — DocumentManager: pending loads + mutex

**`DocumentManager.h`** — add members:
```cpp
#include <mutex>
#include <future>
#include <atomic>

struct PendingLoad {
    std::string chapterId;
    std::future<std::optional<LoadedChapter>> future;
    bool inserted = false;
    bool prepend;
};

std::vector<PendingLoad> pendingLoads_;
std::mutex providerMutex_;
std::atomic<int> fontGeneration_{0};
```

**`TryLoadAdjacent()`** — launch async:
```cpp
bool DocumentManager::TryLoadAdjacent(bool prepend) {
    if (chapters.empty()) return false;

    const std::string& currentId = prepend ? chapters.front().chapterId : chapters.back().chapterId;
    std::string adjacent = prepend ? GetPreviousChapter(currentId) : GetNextChapter(currentId);
    if (adjacent.empty()) return false;

    // Don't queue duplicate pending loads
    for (auto& p : pendingLoads_) {
        if (p.chapterId == adjacent && !p.inserted) return false;
    }

    std::string book; int chapter;
    if (!ParseChapterRef(adjacent, book, chapter)) return false;

    int gen = fontGeneration_.load();
    auto& engine = layoutEngine;
    auto& provider = primaryProvider;
    auto& mutex = providerMutex_;

    auto future = std::async(std::launch::async,
        [&provider, &mutex, &engine, book, chapter, adjacent, gen]()
            -> std::optional<LoadedChapter>
    {
        std::optional<ChapterData> data;
        {
            std::lock_guard<std::mutex> lock(mutex);
            data = provider.LoadChapter(book, chapter);
        }
        if (!data) return std::nullopt;

        // Layout (safe on worker thread with skipCache=true)
        ChapterLayout layout = engine.LayoutChapter(adjacent, *data, true);
        if (gen != engine.GetGeneration()) return std::nullopt; // discarded — layout data stale

        LoadedChapter lc;
        lc.chapterId = adjacent;
        lc.data = std::move(*data);
        lc.layout = std::move(layout);
        lc.height = layout.totalHeight;
        return lc;
    });

    pendingLoads_.push_back({adjacent, std::move(future), false, prepend});
    return true;
}
```

**`Update()`** — process pending loads:
```cpp
void DocumentManager::Update(float dt) {
    // 1. Lerp scroll (same as today)
    float diff = targetScrollY - scrollY;
    if (std::abs(diff) > 0.5f) {
        scrollY += diff * (1.0f - std::exp(-SMOOTH_SPEED * dt));
    } else {
        scrollY = targetScrollY;
    }

    // 2. Process pending loads (at most 1 per frame to spread cost)
    bool loaded = false;
    for (auto& pending : pendingLoads_) {
        if (pending.inserted) continue;
        if (pending.future.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
            continue;

        auto result = pending.future.get();
        pending.inserted = true;
        if (!result) continue;

        auto t0 = std::chrono::steady_clock::now();

        if (pending.prepend) {
            float h = result->height;
            chapters.insert(chapters.begin(), std::move(*result));
            RecalculateChapterPositions();
            targetScrollY += h;
        } else {
            float lastEnd = chapters.empty() ? 0.0f
                : chapters.back().startY + chapters.back().height;
            result->startY = lastEnd;
            chapters.push_back(std::move(*result));
        }

        auto t1 = std::chrono::steady_clock::now();
        float insertMs = std::chrono::duration<float, std::milli>(t1 - t0).count();
        UpdateLoadTime(insertMs);
        eventBus_.Emit(ScrollStopEvent{});
        loaded = true;
        break; // one per frame
    }

    // 3. Clean up finished pendings
    pendingLoads_.erase(std::remove_if(pendingLoads_.begin(), pendingLoads_.end(),
        [](const auto& p) { return p.inserted; }), pendingLoads_.end());

    // 4. Trigger new loads if none just completed
    if (!loaded) {
        if (scrollY <= autoLoadMargin_) {
            TryLoadAdjacent(true);
        }
        float maxScroll = GetTotalHeight() - viewportHeight;
        if (maxScroll < 0.0f) maxScroll = 0.0f;
        if (scrollY >= maxScroll - autoLoadMargin_) {
            TryLoadAdjacent(false);
        }
    }

    UpdateVisibleChapter();
}
```

**`LoadInitialChapter()`** — keep synchronous (nothing pending yet):
```cpp
void DocumentManager::LoadInitialChapter(const std::string& chapterId) {
    chapters.clear();
    pendingLoads_.clear();
    // ... parse ref ...
    {
        std::lock_guard<std::mutex> lock(providerMutex_);
        auto result = primaryProvider.LoadChapter(book, chapter);
        // ... rest of existing code ...
    }
}
```

**`InvalidateLayouts()`** — discard pending loads + bump generation:
```cpp
void DocumentManager::InvalidateLayouts() {
    pendingLoads_.clear();
    fontGeneration_++;
    // ... existing: relayout all chapters ...
}
```

**Verification**: Build + run. Scroll through chapters — no more frame hitches.
Change font size mid-scroll — stale layouts are discarded, re-triggered.

---

### Step 5 — Bump version + tag

Bump MINOR in `CMakeLists.txt` (new feature: async loading + dynamic margin):
```
project(theword VERSION 1.1.0 LANGUAGES C CXX)
```

---

## Summary of file changes

| File | Action | Notes |
|------|--------|-------|
| `src/event/Events.h` | Modify | Add `ScrollStopEvent` (+3 lines) |
| `src/document/DocumentManager.h` | Modify | Async members, mutex, generation, pending loads, margin tracking (+~25 lines) |
| `src/document/DocumentManager.cpp` | Modify | Core rewrite of `Update`, `TryLoadAdjacent`, new timing methods (+~90 lines, -~20 lines) |
| `src/input/InputHandler.cpp` | Modify | Subscribe to `ScrollStopEvent` (+4 lines) |
| `src/text/LayoutEngine.h` | Modify | Add `skipCache` param to `LayoutChapter`, `GetGeneration()`, `layoutGeneration_` atomic (+~6 lines) |
| `src/text/LayoutEngine.cpp` | Modify | Guard cache write on `skipCache`, increment generation in invalidation methods (+~7 lines) |
| `CMakeLists.txt` | Modify | Bump version to 1.1.0 |

## Verification after each step

```bash
cmake --build build --parallel 2>&1 | tail -10
./build/theword
```

Test scenarios:
1. **Prepend fix**: Scroll up to Genesis 1 → no snap, chapter loads smoothly
2. **Velocity fix**: Scroll to bottom with hard flick → no lurch after append
3. **Dynamic margin**: Watch log for `autoLoadMargin_` values changing
4. **Async loading**: Scroll through chapters → no frame stutters
5. **Font resize while loading**: Change font size mid-load → stale layout discarded, re-triggers
6. **Scrollbar**: Smooth, no jumps when content grows

## Rollback

Each step is a separate commit. `git checkout HEAD -- <files>` reverts any single step.
