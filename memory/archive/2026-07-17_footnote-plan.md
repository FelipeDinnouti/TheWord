# Feature 4: Footnote Display — Execution Plan

> Working document for v1.7.0-alpha | Reference during development
> Version 2 — corrected after code review (2026-07-16)

---

## Phase 1: Data Layer

### 1.1 `ChapterProvider.h` — 4 additions

- `struct Footnote {int verseId; std::string callerRef; std::string text;};`
- `SegmentType::FootnoteMarker` — new enum value
- `ChapterData::footnotes` — `std::vector<Footnote> footnotes`
- `Span::footnoteIndex` — `int footnoteIndex = -1` (default), links to `footnotes[i]`

### 1.2 `Theme.h` — 3 constexpr colors

```cpp
DOC_FOOTNOTE_CALLER    {100, 100, 180, 255}   // [n] marker
DOC_FOOTNOTE_POPUP_BG  {240, 240, 250, 240}   // popup bg
DOC_FOOTNOTE_POPUP_TEXT  {30, 30, 50, 255}    // popup text
```

### 1.3 `USFMParser.h/cpp` — StripFootnotes → ExtractFootnotes

**Approach:** Bulk extraction from raw USFM content, distribute to chapters post-parse.

| Item | Detail |
|------|--------|
| New sig | `std::string ExtractFootnotes(const std::string& usfmText, std::vector<std::pair<int, Footnote>>& outFootnotes) const;` — returns cleaned text, populates `(chapterNum, Footnote)` buffer |
| Chapter tracking | Scan raw USFM text sequentially; detect `\c N` to track current chapter, `\v M` for verse context |
| On `\f...\f*` | Extract `\fr` content (callerRef), `\ft` content (text). Push `(currentChapter, Footnote{currentVerse, callerRef, text})`. |
| Return | Cleaned text = input with footnote blocks removed (same output format as current `StripFootnotes`) |
| Edge cases | `\f + \fr ... \f*` (no `\ft`) — use whole interior as text. `\f ... \f*` (no `\fr`) — no caller ref. Nested markers (`\add`, `\wj`, `\it`) — stripped by `StripInlineMarkers()` before storage. |

**Integration in `ParseBook()` (line 344):**
```diff
- content = StripFootnotes(content);
+ std::vector<std::pair<int, Footnote>> footnoteBuf;
+ content = ExtractFootnotes(content, footnoteBuf);
  content = StripInlineMarkers(content);
```

**After chapter-building loop (around line 406):**
```cpp
for (auto& ch : chapters) {
    for (auto& [chNum, fn] : footnoteBuf) {
        if (chNum == ch.chapterNum) {
            ch.footnotes.push_back(std::move(fn));
        }
    }
}
```

### 1.4 `BibleClient.cpp` — StripFootnotes → ExtractFootnotes

**Self-contained extraction (no `currentVerse` parameter needed).**

| Item | Detail |
|------|--------|
| New sig | `std::string ExtractFootnotes(const std::string& html, ChapterData& data);` — returns cleaned HTML, pushes `Footnote`s directly to `data.footnotes` |
| Algorithm | Same HTML stack-depth scan as current `StripFootnotes`. Additionally: track `yv-v` tags (using `ParseVerseNumber()`) to know current verse. On entering `yv-n f` block: capture inner HTML; on exit, extract `<span class="fr">` (callerRef) and `<span class="ft">` (footnote text). Create `Footnote{currentVerse, callerRef, text}` and push to `data.footnotes`. |
| Return | Cleaned HTML with `yv-n f` blocks removed (same output as current `StripFootnotes`) |
| Callsites | Both `ParseParagraphContent()` (line 169) and `ParsePoetryLine()` (line 247): `ExtractFootnotes(html, data)` instead of `StripFootnotes(html)`. Both already have `ChapterData& data`. |

---

## Phase 2: Layout + Rendering

### 2.1 `LayoutEngine.cpp` — Insert `[n]` marker spans

In `LayoutWords()`, inside the VerseNumber insertion block (after `x += vnWidth;` at line 208, before the `}` at line 209), add:

```cpp
// Insert footnote markers for this verse
for (size_t fi = 0; fi < data.footnotes.size(); ++fi) {
    if (data.footnotes[fi].verseId == currentVerse) {
        Span fnSpan;
        fnSpan.text = "[" + std::to_string(fi + 1) + "]";
        fnSpan.type = SegmentType::FootnoteMarker;
        fnSpan.footnoteIndex = static_cast<int>(fi);
        fnSpan.verseId = currentVerse;
        fnSpan.bookId = data.bookId;
        fnSpan.chapterNum = data.chapterNum;
        fnSpan.startWord = -1;          // not a word
        fnSpan.endWord = -1;            // not a word
        float fnWidth = MeasureTextEx(smallFont_, fnSpan.text.c_str(), smallSize_, 1).x;
        fnSpan.x = x;
        fnSpan.y = y;
        fnSpan.width = fnWidth;
        fnSpan.height = smallSize_;
        currentLine.spans.push_back(fnSpan);
        x += fnWidth;
        // NO extra spaceWord here — the word-spacing code (lines 227-228) handles
        // spacing before the first word of the verse automatically
    }
}
```

**Important:** `startWord = -1` ensures `HitTestLine()` returns -1 for taps on footnote markers, not 0 (which would be treated as a valid word).

Also add `case SegmentType::FootnoteMarker:` to `LayoutChapter()` switch — falls through to `LayoutWords()` path (for completeness).

### 2.2 `Renderer.cpp` — Draw markers

In `DrawSpan()` switch add:

```cpp
case SegmentType::FootnoteMarker:
    color = theme::DOC_FOOTNOTE_CALLER;
    drawSize = smallSize_;
    useFont = smallFont;
    break;
```

Apply superscript Y offset — expand existing VerseNumber condition (line 91):
```cpp
if (span.type == SegmentType::VerseNumber || span.type == SegmentType::FootnoteMarker) {
    pos.y = screenY - drawSize * 0.25f;
}
```

---

## Phase 3: Interaction

### 3.1 `InputHandler.h` — 2 new callbacks

```cpp
std::function<void(int footnoteIndex)> onFootnoteTap;
std::function<int(float, float)> hitTestFootnoteFn;  // returns footnoteIndex or -1
```

### 3.2 `InputHandler.cpp` — Hit test in FSM

In `RunUnifiedFSM()` → `Pending → justReleased` path (lines 202-216):

**Before existing word/tap-empty logic:**
```cpp
if (justReleased && !didScroll_) {
    // Check footnote hit first (higher z-order)
    if (hitTestFootnoteFn) {
        int fi = hitTestFootnoteFn(pressStartPos.x, pressStartPos.y);
        if (fi >= 0 && onFootnoteTap) {
            onFootnoteTap(fi);
            pressState = PressState::Idle;
            return;
        }
    }
    // ... existing word/double/tap-empty checks ...
```

Since footnote marker spans have `startWord = -1`, `pressStartHit.wordId` will be -1 when tapping a marker — they naturally fall through to the `wordId < 0` path. The footnote check runs before the tap-empty handler so markers take priority over empty-space taps.

### 3.3 `App.cpp` — Wire hitTestFootnoteFn + capture

In `App::Init()`, after existing `hitTestFn` + `isHighlightedFn`, add:

```cpp
// Stores contentTop for capture by hitTestFootnoteFn lambda
float contentTopCapture = contentTop;

auto hitTestFootnoteFn = [this, contentTopCapture](float x, float y) -> int {
    std::vector<std::pair<theword::data::Span, float>> spans;
    docManager_->GetVisibleSpans(spans);
    for (const auto& [span, docY] : spans) {
        if (span.type != SegmentType::FootnoteMarker || span.footnoteIndex < 0) continue;
        float screenY = docY - docManager_->GetScrollY() + contentTopCapture;
        if (y >= screenY && y <= screenY + span.height &&
            x >= span.x && x <= span.x + span.width) {
            return span.footnoteIndex;
        }
    }
    return -1;
};
```

Note: uses `span.height` (actual rendered height) instead of `smallSize_` (not accessible from App scope).

Wire the tap callback:
```cpp
inputHandler_->onFootnoteTap = [this](int fi) {
    auto* cd = docManager_->GetCurrentChapterData();
    if (cd && fi >= 0 && fi < static_cast<int>(cd->footnotes.size())) {
        const auto& fn = cd->footnotes[fi];
        std::string text = fn.callerRef.empty() ? fn.text : fn.callerRef + " " + fn.text;
        uiManager_->ShowFootnotePopup(text, GetMousePosition());
    }
};
```

Pass `hitTestFootnoteFn` to InputHandler constructor:
```diff
- inputHandler_ = std::make_unique<InputHandler>(*eventBus_, hitTestFn, isHighlightedFn);
+ inputHandler_ = std::make_unique<InputHandler>(*eventBus_, hitTestFn, isHighlightedFn, hitTestFootnoteFn);
```

### 3.4 `InputHandler.h` — Update constructor

Add `hitTestFootnoteFn` to constructor params:
```cpp
InputHandler(theword::event::EventBus& eventBus,
             std::function<HitInfo(float, float)> hitTestFn = nullptr,
             std::function<bool(int)> isHighlightedFn = nullptr,
             std::function<int(float, float)> hitTestFootnoteFn = nullptr);
```

Store as private member alongside `hitTestFn` and `isHighlightedFn`.

### 3.5 `UIManager.h/cpp` — FootnotePopup

```cpp
void ShowFootnotePopup(const std::string& text, Vector2 position);
void DrawFootnotePopup();
void HideFootnotePopup();
bool IsFootnotePopupActive() const;
```

Private: `std::string footnoteText_`, `Vector2 footnotePos_`, `bool footnotePopupActive_ = false`.

`DrawFootnotePopup()`: if active, draw rounded rect near tap position with wrapped text. Same layout pattern as Toast but persistent until dismissed. Use `GetFontDefault()`, 14 * dpiScale font, max-width ~50% screen width, `DOC_FOOTNOTE_POPUP_BG`/`_TEXT` colors.

### 3.6 `App.cpp` — Wire dismiss (Escape + tap-away)

**Escape dismiss** — `OnDismiss()` (line 700), check footnote popup BEFORE radial menu:
```cpp
bool App::OnDismiss() {
    if (uiManager_->IsFootnotePopupActive()) {
        uiManager_->HideFootnotePopup();
        return true;
    }
    if (uiManager_->IsRadialMenuActive()) {
        uiManager_->HideRadialMenu();
        accumStartWord_ = -1;
        accumEndWord_ = -1;
        return true;
    }
    return false;
}
```

**Tap-away dismiss** — `OnTap()` (line 530), at the very top:
```cpp
void App::OnTap(...) {
    if (uiManager_->IsFootnotePopupActive()) {
        uiManager_->HideFootnotePopup();
        return;  // consume — popup takes priority
    }
    // ... existing radial menu + word tap logic ...
```

**Tap-away dismiss** — `OnTapEmpty()` (line 622), at the very top:
```cpp
void App::OnTapEmpty(Vector2 pos) {
    if (uiManager_->IsFootnotePopupActive()) {
        uiManager_->HideFootnotePopup();
        return;
    }
    // ... existing radial menu click logic ...
```

**Draw cycle** — `MainLoop()`: call `uiManager_->DrawFootnotePopup()` after `DrawToast()`:
```diff
  uiManager_->DrawRadialMenu();
  uiManager_->DrawToast();
+ uiManager_->DrawFootnotePopup();  // drawn on top (last)
  renderer_->DrawFpsCounter(...);
```

---

## Phase 4: Tests

### 4.1 BibleClient test (`test_main.cpp:353`)
- Rename to `"BibleClient extracts footnotes from HTML"`
- Assert: `CHECK(result->footnotes.size() == 1)`
- Assert: `CHECK(result->footnotes[0].text == "footnote")`
- Keep: `CHECK_FALSE(hasFootnote)` — words still shouldn't contain footnote text

### 4.2 New USFM footnote test
- USFM string with `\f + \fr 1:1 \ft Note text.\f*`
- Parse, verify `chapter.footnotes.size() == 1`, text == "Note text.", words don't contain "Note"

### 4.3 LayoutEngine test
- `ChapterData` with footnotes → `LayoutChapter()` → verify lines contain `FootnoteMarker` spans with correct `footnoteIndex`

---

## File Change Summary (13 files)

| File | What |
|------|------|
| `src/data/ChapterProvider.h` | +Footnote struct, +SegmentType::FootnoteMarker, +footnotes vec, +Span::footnoteIndex |
| `src/core/Theme.h` | +3 footnote colors |
| `src/data/USFMParser.h` | StripFootnotes → ExtractFootnotes decl change |
| `src/data/USFMParser.cpp` | Rewrite impl; update ParseBook() for distribution |
| `src/data/BibleClient.cpp` | Replace StripFootnotes with self-contained ExtractFootnotes (tracks yv-v) |
| `src/text/LayoutEngine.cpp` | Insert [n] markers in LayoutWords(); startWord=-1; no extra spacing |
| `src/renderer/Renderer.cpp` | Add FootnoteMarker case; superscript Y |
| `src/input/InputHandler.h` | +onFootnoteTap + hitTestFootnoteFn (member + ctor param) |
| `src/input/InputHandler.cpp` | Hit test in RunUnifiedFSM() before word check |
| `src/renderer/UIManager.h` | +FootnotePopup API |
| `src/renderer/UIManager.cpp` | FootnotePopup implementation |
| `src/app/App.cpp` | Wire hitTestFootnoteFn (lambda, contentTop capture, GetVisibleSpans out-param, span.height); wire onFootnoteTap; tap-away in OnTap/OnTapEmpty; Escape in OnDismiss; DrawFootnotePopup in MainLoop |
| `tests/test_main.cpp` | Update BibleClient test; add USFM + LayoutEngine tests |
