# Phase 13 — Highlight Browser

> **Status**: Planned | Dependencies: Phases 1-12 complete
>
> This phase implements the Highlight Browser: a full-screen view that lets users browse all their highlights filtered by color, see verse references and snippet text, and tap to navigate directly to the highlighted verse in the Reader.

---

## Overview

The CenterMenu already has a "Highlights" option (`CenterMenu.cpp:152`) that is currently a no-op (`break;`). Phase 13 wires it up and completes the highlight workflow: **create → browse → navigate-to**.

```
CenterMenu
  └── Highlights → HighlightBrowserScreen
                    ├── color swatch filter
                    ├── scrollable match list (ref + snippet)
                    └── tap item → Reader at verse
```

### Key Deliverables

1. **Data model** — `Highlight` struct gains `bookId`, `chapterNum`, `verseStart`, `verseEnd`, `verseText`
2. **Highlighter** — `EndSelection()` captures chapter context; `GetHighlightsByType()` filter API
3. **Persistence** — Schema migration (`ALTER TABLE` for 5 columns), update save/load
4. **HighlightBrowserScreen** — New full-screen with color swatch filter + scrollable item list
5. **Reader navigation** — Handle `NavigateToHighlightEvent` to scroll to target word
6. **CenterMenu wiring** — `case 2` pushes HighlightBrowserScreen
7. **Tests** — Reference field population, filter by color, persistence migration, navigation event

---

## Data Model

### `src/highlight/PersistenceInterface.h`

Add reference + snippet fields to `Highlight`:

```cpp
struct Highlight {
    int id;
    int startWord;
    int endWord;
    int typeId;
    std::string providerName;
    // New — Phase 13:
    std::string bookId;        // e.g. "GEN"
    int chapterNum;            // e.g. 1
    int verseStart;            // first verse in range
    int verseEnd;              // last verse in range
    std::string verseText;     // snippet of highlighted words (~80 chars)
};
```

`verseText` is populated at highlight-creation time from the chapter's word data. This avoids needing to load the chapter later just to display the snippet. Truncated with `"..."` if > 80 chars.

### `src/event/Events.h`

Add navigation event:

```cpp
struct NavigateToHighlightEvent {
    std::string chapterRef;  // e.g. "GEN.3"
    int wordId;              // target word to scroll into view
};
```

---

## Highlighter Changes

### `src/highlight/Highlighter.h`

New method:

```cpp
std::vector<const Highlight*> GetHighlightsByType(int typeId) const;
```

### `src/highlight/Highlighter.cpp`

**`EndSelection()`** — capture chapter context and populate reference fields.

The event currently doesn't carry chapter context. Two options:

**Option A (recommended):** Pass chapter context through the existing `SelectionEvent`. Add optional fields:

```cpp
struct SelectionEvent {
    enum class Action { Start, Update, End, Cancel } action;
    int startWordId;
    int endWordId;
    // Optional context for End:
    std::string bookId;
    int chapterNum;
};
```

`EndSelection()` uses `bookId`/`chapterNum` and scans `words[]` between `startWord`/`endWord` to determine `verseStart`/`verseEnd` from each word's `verseId`. Builds `verseText` by concatenating word texts in the range.

**Option B:** Store the current chapter context in the Highlighter as members, set by a `SetChapterContext()` call before selection events flow. Simpler but requires App or InputHandler to call it.

Go with **Option A** — keeps it event-driven.

```cpp
void Highlighter::EndSelection() {
    if (!selecting) return;
    selecting = false;
    int start = std::min(selectionStart, selectionEnd);
    int end = std::max(selectionStart, selectionEnd);
    if (start < 0) return;

    Highlight h;
    h.id = nextId++;
    h.startWord = start;
    h.endWord = end;
    h.typeId = activeTypeId;
    h.providerName = currentProvider;
    h.bookId = pendingBookId_;
    h.chapterNum = pendingChapterNum_;
    // Resolve verse range from the last EndSelection event
    // (verseStart/verseEnd set from the bookId/chapterNum + word scanning)
    // Build verseText from concatenation of word texts at the time
    highlights.push_back(h);
    persistence.SaveHighlight(h);
}
```

**`GetHighlightsByType(int typeId)`:**

```cpp
std::vector<const Highlight*> Highlighter::GetHighlightsByType(int typeId) const {
    std::vector<const Highlight*> result;
    for (const auto& h : highlights) {
        if (h.typeId == typeId) {
            result.push_back(&h);
        }
    }
    return result;
}
```

(Returns pointers — caller must not outlive the Highlighter. Fine for synchronous draw.)

---

## Persistence Schema Migration

### `src/persistence/PersistenceManager.cpp`

Add migration in `InitSchema()` (same pattern as existing `provider_name` migration):

```cpp
// Phase 13 — highlight reference fields
const char* migrateRefs = "ALTER TABLE highlights ADD COLUMN book_id TEXT";
sqlite3_exec(db, migrateRefs, nullptr, nullptr, &err);
sqlite3_free(err);

const char* migrateChapter = "ALTER TABLE highlights ADD COLUMN chapter_num INTEGER";
sqlite3_exec(db, migrateChapter, nullptr, nullptr, &err);
sqlite3_free(err);

const char* migrateVStart = "ALTER TABLE highlights ADD COLUMN verse_start INTEGER";
sqlite3_exec(db, migrateVStart, nullptr, nullptr, &err);
sqlite3_free(err);

const char* migrateVEnd = "ALTER TABLE highlights ADD COLUMN verse_end INTEGER";
sqlite3_exec(db, migrateVEnd, nullptr, nullptr, &err);
sqlite3_free(err);

const char* migrateText = "ALTER TABLE highlights ADD COLUMN verse_text TEXT";
sqlite3_exec(db, migrateText, nullptr, nullptr, &err);
sqlite3_free(err);
```

**`LoadHighlights()`** — read 5 new columns. Existing highlights get `NULL` → C++ defaults (empty string, 0). Handle gracefully in browser (show "Unknown").

**`SaveHighlight()`** — bind 5 new parameters.

---

## HighlightBrowserScreen

### New files: `src/ui/HighlightBrowserScreen.h`, `src/ui/HighlightBrowserScreen.cpp`

Full-screen implementing `Screen` interface.

```cpp
class HighlightBrowserScreen : public Screen {
public:
    HighlightBrowserScreen(const Font& font, float fontSize,
                           NavigationStack& navStack,
                           theword::event::EventBus& eventBus,
                           const theword::highlight::Highlighter& highlighter,
                           const theword::core::UIScale& uiScale);
    void Draw() override;
    bool HandleInput(float deltaTime) override;
    const char* GetTitle() const override { return "Highlights"; }

private:
    const Font& font_;
    float fontSize_;
    NavigationStack& navStack_;
    theword::event::EventBus& eventBus_;
    const theword::highlight::Highlighter& highlighter_;
    const theword::core::UIScale& uiScale_;

    int activeColorId_ = 0;       // 0 = none selected
    float scrollOffset_ = 0.0f;

    void DrawColorFilter();
    void DrawHighlightList();
    void OnItemTapped(int index);
    void NavigateToHighlight(const Highlight& h);
};
```

### Layout

```
┌──────────────────────────────┐
│  ← Back    Highlights        │  DrawHeaderBar(title, hasBack=true)
├──────────────────────────────┤
│  Color:  ■ ■ ■ ■ ■           │  filter swatch row (or "All")
├──────────────────────────────┤
│  Gen 1:3                     │  reference title (bookId.chapterNum:verseStart)
│  No princípio criou Deus...  │  verse text snippet (80 chars max + "...")
│                              │
│  Gen 1:5                     │
│  E chamou Deus à luz...      │
│                              │
│  (empty state if no matches) │  "No highlights of this color."
└──────────────────────────────┘
```

### Behavior

- **No color selected** (`activeColorId_ == 0`): Show all highlights across all colors, or show prompt "Select a color to browse"
- **Tap swatch**: Set `activeColorId_`, reset `scrollOffset_` to 0
- **Tap highlighted swatch again**: Deselect (show all / prompt)
- **List items**: Built from `highlighter_.GetHighlightsByType()` if filtered, or `highlighter_.GetHighlights()` if showing all
- **Each item** = reference line (`Book Chapter:VerseRange`) + snippet line (text truncated, highlight color shown as background rect behind snippet words)
- **Tap item**: `navStack_.PopAll()` → emit `NavigateToHighlightEvent{chapterRef, wordId}` → ReaderScreen handles it
- **Scroll**: Mouse wheel / touch drag on list area
- **Back**: ← Back button in header bar, or Escape

### Drawing details

Reuse existing components from `components.h`:
- `DrawHeaderBar(font, size, "Highlights", true)` for top
- `DrawColorSwatches(font, rect, types, n, activeId, mouse)` for filter (same as Settings screen)
- `DrawListItem(font, rect, title, subtitle, mouse)` for each match

Color swatch: Same 5 pastel colors from `Highlighter::GetTypes()`.

---

## ReaderScreen: Handle Navigation

### `src/ui/ReaderScreen.h/cpp`

Subscribe to `NavigateToHighlightEvent` in constructor:

```cpp
eventBus_.On<theword::event::NavigateToHighlightEvent>(
    [this](const auto& e) { OnNavigateToHighlight(e); });
```

Handler:

```cpp
void ReaderScreen::OnNavigateToHighlight(const theword::event::NavigateToHighlightEvent& e) {
    docManager_.LoadInitialChapter(e.chapterRef);
    // After load, search layout for line containing e.wordId
    // Set scroll position so that line is at contentTop
    // (Implemented as a delayed action, since load is async)
    pendingNavigateWordId_ = e.wordId;
}
```

After each chapter load, check `pendingNavigateWordId_`:

```cpp
if (pendingNavigateWordId_ >= 0) {
    // Find the line in the latest ChapterLayout that contains this word ID
    float targetY = FindLineYForWord(pendingNavigateWordId_);
    if (targetY >= 0.0f) {
        docManager_.ScrollTo(targetY);
    }
    pendingNavigateWordId_ = -1;
}
```

`FindLineYForWord()` iterates the current chapter's layout lines and their spans, looking for one where `span.startWord <= wordId <= span.endWord`. Returns that line's Y position.

---

## CenterMenu Wiring

### `src/ui/CenterMenu.cpp`

Replace `case 2: break;` with context from `OnActivate()` or directly push:

```cpp
case 2: // Highlights
    navStack_.Push(std::make_unique<HighlightBrowserScreen>(
        font_, fontSize_, navStack_, eventBus_,
        highlighter_, uiScale_
    ));
    return;
```

Requires forward-declaring `HighlightBrowserScreen.h`.

---

## InMemoryStorage Updates

### `src/highlight/InMemoryStorage.h/cpp`

Update test storage to handle the new `Highlight` fields. The `SaveHighlight` stores by ID (existing behavior). Fields just pass through with the struct — no extra logic needed since `InMemoryStorage` uses `std::vector<Highlight>`.

---

## Tests

### New test cases in `tests/test_main.cpp`

| # | Test | Description |
|---|------|-------------|
| 1 | `Highlighter stores reference fields` | `EndSelection()` with `bookId="GEN"`, `chapterNum=1`. Verify `GetHighlights()[0].bookId`, `.chapterNum`, `.verseStart`, `.verseEnd` are correct. |
| 2 | `Highlighter verseText populated` | After EndSelection, verify `verseText` contains concatenated words from the range. |
| 3 | `GetHighlightsByType filters correctly` | Create highlights of type 1 and 2. Filter by type 1, verify count = 1. Filter by type 2, verify count = 1. Filter by type 99, verify count = 0. |
| 4 | `PersistenceManager saves/loads ref fields` | `SaveHighlight` with ref fields → `LoadHighlights` → verify round-trip. |
| 5 | `PersistenceManager schema migration` | Create DB without new columns (simulate old schema), call `InitSchema`, verify migration succeeds and existing rows have NULL → empty defaults. |
| 6 | `NavigateToHighlightEvent emitted` | Verify the event struct carries correct `chapterRef` and `wordId`. |

---

## File Change Inventory

| File | Action |
|------|--------|
| `src/highlight/PersistenceInterface.h` | Add `bookId`, `chapterNum`, `verseStart`, `verseEnd`, `verseText` to `Highlight` |
| `src/highlight/Highlighter.h` | Add `GetHighlightsByType()` |
| `src/highlight/Highlighter.cpp` | Update `EndSelection()` for ref fields; implement `GetHighlightsByType()` |
| `src/highlight/InMemoryStorage.h` | No changes needed (templated on `Highlight` struct) |
| `src/highlight/InMemoryStorage.cpp` | No changes needed |
| `src/persistence/PersistenceManager.cpp` | Schema migration (5 ALTER TABLE), update `LoadHighlights`/`SaveHighlight` |
| `src/event/Events.h` | Add `bookId`, `chapterNum` to `SelectionEvent`; add `NavigateToHighlightEvent` |
| `src/ui/HighlightBrowserScreen.h` | **Create** |
| `src/ui/HighlightBrowserScreen.cpp` | **Create** |
| `src/ui/ReaderScreen.h` | Add `OnNavigateToHighlight`, `pendingNavigateWordId_`, `FindLineYForWord()` |
| `src/ui/ReaderScreen.cpp` | Implement event handler + scroll-to-word logic |
| `src/ui/CenterMenu.cpp` | Wire case 2 to push HighlightBrowserScreen |
| `CMakeLists.txt` | Add `HighlightBrowserScreen.h/cpp` to `UI_SOURCES` |
| `tests/test_main.cpp` | Add 6 new test cases |

---

## Verification

```bash
rm -rf build && cmake -B build -DCMAKE_BUILD_TYPE=Release -G "Unix Makefiles"
cmake --build build --parallel

# Unit tests — all existing + new must pass
./build/theword_test

# Manual smoke test:
# 1. Start theword, navigate to a chapter
# 2. Select some words (drag) → highlight created
# 3. Tap book code → CenterMenu → "Highlights"
# 4. See HighlightBrowser screen with color swatches
# 5. Tap a color swatch → matching highlights appear
# 6. Tap a highlight item → pops back to Reader at the verse
# 7. Create highlights with different colors → filter works
# 8. Delete a highlight, go back to browser → no longer listed
# 9. Quit and restart → highlights persist, ref fields survive restart

# No regression: all existing UI (book nav, settings, font, scroll) unaffected
```
