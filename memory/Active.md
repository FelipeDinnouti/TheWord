# Active

> Current version: v1.7.0-alpha
>
> Archive: `memory/archive/2026-07-16_v1.7.0-alpha-complete.md`
>
> Previous: `memory/archive/2026-07-14_v1.6.x-complete.md`

## Workstream: v1.7.0-alpha — Copy Verse, Footnotes, Open Where You Left Off, Immersive Mode

**NVI License Decision:** Deferred. Biblica Fast-Track License has a 12-month deployment clause (§13.E) that a solo dev cannot meet. Will sign when within ~6 months of a production release. See `the-word-docs/xx-user-notes/License Agreement BIBLICA.md` for full analysis.

**Theme:** Reading Experience — copy fidelity, footnote support, session continuity, clean reading.

### Implementation Order (smallest → largest)

| # | Feature | Scope | Status |
|---|---------|-------|--------|
| 1 | **Copy Verse Polish** | 4 files, ~50 lines | ✅ Complete |
| 2 | **Immersive / Clean Mode** | 6 files, ~80 lines | ✅ Complete |
| — | **Verse Flow Fix** | 1 file, ~30 lines | ✅ Complete |
| — | **Bible ID Fix (129→3034)** | 1 line | ✅ Complete |
| 3 | **Open Where You Left Off** | 1 file, ~15 lines | ✅ Complete |
| 4 | **Footnote Display** | 13 files, ~400 lines | ✅ Complete |

### Design Decisions (All Features)

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Immersive flag ownership | `App` member, passed as `bool&` | Matches existing `versionOnline_` pattern; accessible from HandleShortcuts, SettingsScreen, ReaderScreen |
| Immersive filter location | `ReaderScreen::Draw()` post-`GetVisibleSpans()` | Simple, no DocumentManager changes needed; toggle only affects reader view |
| `key::I` | Add to `Config.h` namespace | Follows existing G/S/A pattern |
| `last_scroll` save triggers | 3 locations: NavigateEvent, Android pause, MainLoop pre-exit | Covers all exit paths without a dedicated lifecycle event |
| `last_scroll` restore timing | Deferred to first frame of MainLoop | Sync chapter load resets scrollY to 0; need layout ready before ScrollTo |
| Footnote hit-test | New `onFootnoteTap(int footnoteIndex)` callback in InputHandler | Most decoupled; doesn't pollute HitInfo with footnote metadata | ✓ |
| FootnotePopup lifecycle | Owned by UIManager (like RadialMenu), drawn post-radial | Same overlay pattern; tap-away dismiss via onTap/onDismiss | ✓ |
| USFM footnote extraction | Two-pass: pre-extract from raw content tracking \c/\v markers, then distribute to ChapterData during chapter building | Avoids multi-line issues; keeps line-by-line parsing clean | ✓ |

---

## Feature 1: Copy Verse Polish

**Format:** `Book Chapter:VerseStart-VerseEnd\n\n<text>` — citation with two line breaks, then body text.
**Shortcut:** Ctrl+C on desktop (only when a text selection is active).
**Feedback:** Toast popup "Copied!" auto-dismissing after 1.5s.

### Checklist

- [x] **Update `AssembleSelectedText()`** (`App.cpp:54-65`)
  - Scan `data.words` within `[s, e]` for min/max `verseId`
  - Build citation: `BOOK_NAMES_PT[idx] + " " + ch + ":" + firstVerse` + (`"-" + lastVerse` if multi-verse) + `"\n\n"`
  - Fallback: if `FindBookIndex()` returns -1, use `data.bookId` raw code
  - Skip verse range lookup if `s`/`e` out of bounds
- [x] **Add `CopySelection()` helper** to `App.h`/`App.cpp`
  - `void CopySelection(const ChapterData& data, int startWord, int endWord);`
  - Calls `AssembleSelectedText()` → `platform::SetClipboard()` → `uiManager_->ShowToast("Copied!")`
  - Shared between radial menu copy and Ctrl+C shortcut
- [x] **Replace inline copy in `OnTap()`** (lines 495-502): call `CopySelection()` instead of raw `AssembleSelectedText` → `SetClipboard`
- [x] **Replace inline copy in `OnTapEmpty()`** (lines 585-590): same replacement
- [x] **Add Ctrl+C shortcut in `HandleShortcuts()`**
  - `if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_C) && accumStartWord_ >= 0)`
  - Get current chapter data via `docManager_->GetCurrentChapterData()`
  - Call `CopySelection(*cd, accumStartWord_, accumEndWord_)`
- [x] **Add Toast API to `UIManager.h`**
  - `void ShowToast(const std::string& text);`
  - `void DrawToast();`
  - Private: `std::string toastText_`, `double toastStartTime_ = 0.0`
  - `static constexpr double TOAST_DURATION = 1.5;`
- [x] **Implement Toast in `UIManager.cpp`**
  - `ShowToast()`: set `toastText_`, `toastStartTime_ = GetTime()`
  - `DrawToast()`: if `elapsed > TOAST_DURATION` → clear; else centered at 70% screen height, semi-transparent black rounded rect, white `GetFontDefault()` text
- [x] **Wire Toast into draw loop** in `MainLoop()`: call `uiManager_->DrawToast()` after `DrawRadialMenu()` but before `EndDrawing()`
- [x] **Verify**: build desktop, copy via radial menu button + Ctrl+C shortcut, confirm toast appears and dismisses

---

## Feature 2: Immersive / Clean Mode

**Hides:** Verse numbers, chapter labels, section headings. (BookTitle stays visible for context.)
**Toggle:** 'I' hotkey (desktop) + SettingsScreen toggle. **Persists** across sessions.

### Checklist

- [x] **Add `key::I` to `Config.h`** — Desktop KEY_I, Android AKEYCODE_I = 37
- [x] **Add `bool immersiveMode_` to `App.h`** — alongside `versionOnline_`
- [x] **Pass to ReaderScreen** — `bool& immersiveMode` parameter, stored as member
- [x] **Pass to SettingsScreen** — `bool& immersiveMode` parameter, stored as member
- [x] **Update App.cpp callsites** — ReaderScreen + SettingsScreen creation pass `immersiveMode_`
- [x] **Load preference in `App::Init()`** — `persistence_->GetPreference("immersive_mode", "0")`
- [x] **Filter spans in `ReaderScreen::Draw()`** — `std::remove_if` erases VerseNumber/ChapterLabel/SectionHeading
- [x] **Add 'I' shortcut in `HandleShortcuts()`** — toggle + persist
- [x] **Add SettingsScreen toggle** — ON/OFF buttons toggle + save preference
- [x] **Merge consecutive VerseText segments** in LayoutChapter() — verses flow inline
- [x] **Fix Bible ID** — 129 (403) → 3034 (BSB)
- [x] **Verify**: toggle works, persists across restart

---

## Feature 3: Open Where You Left Off

**Saves** scroll position at 3 trigger points: chapter navigation, Android pause, desktop quit.
**Restores** on startup after chapter is loaded and layout is ready.

### Checklist

- [x] **Add `float pendingScrollY_ = -1.0f`** to `App.h`
- [x] **Save in `NavigateEvent` handler** — `persistence_->SetPreference("last_scroll", ...)` alongside existing `last_chapter` save
- [x] **Save on Android pause** — alongside existing `lifecycle_scroll` save
- [x] **Save on desktop pre-exit** — before `break` when quitting, guarded by `navStack_->IsOnRoot()`
- [x] **Load at startup** — in `App::Init()` after `lifecycle_scroll` restore: read `last_scroll` → `pendingScrollY_`
- [x] **Apply deferred scroll** — at top of `MainLoop()` first iteration: `ScrollTo(pendingScrollY_)`, reset to -1
- [ ] **Verify**: scroll in chapter → close → reopen → restored to same position; navigate to new chapter → close → reopen → restored (should be new chapter's scroll, not old)

---

## Feature 4: Footnote Display

**Presentation:** `[n]` superscript markers inline, tap → popup with footnote text, tap-away → dismiss.
**Sources:** Both USFM (offline files) and BibleClient (online HTML API).
**Complexity:** Touches 5 modules — data, parser, layout, renderer, input.

### Data Flow

```
ChapterProvider structs              ← Footnote, SegmentType::FootnoteMarker, Span::footnoteIndex
  → USFMParser / BibleClient         ← ExtractFootnotes() → ChapterData::footnotes
    → LayoutEngine::LayoutChapter()  ← Insert [n] marker spans linked to footnotes
      → Renderer::DrawSpan()         ← Render markers, superscript style
        → InputHandler::FSM          ← Hit-test markers → onFootnoteTap
          → UIManager::FootnotePopup ← Show footnote text, dismiss on tap-away
```

### Checklist — Done ✓

All items below completed in a single implementation pass.

**Data Layer:** Footnote struct, SegmentType::FootnoteMarker, footnotes vector, Span::footnoteIndex added to ChapterProvider.h. Theme colors added.

**USFMParser:** Replaced StripFootnotes with ExtractFootnotes — tracks `\c N`/`\v M` in raw text, extracts `\fr`/`\ft` from `\f...\f*` blocks, distributes to chapters post-parse. ParseBook() updated.

**BibleClient:** Replaced StripFootnotes with self-contained ExtractFootnotes — tracks `yv-v` tags internally for verse context, extracts `fr`/`ft` spans from `yv-n f` blocks, pushes directly to ChapterData::footnotes. Callers in ParseParagraphContent and ParsePoetryLine updated.

**LayoutEngine:** FootnoteMarker case in LayoutChapter switch. In LayoutWords, `[n]` marker spans inserted after verse number, with startWord=-1 for correct hit-testing, no extra spacing.

**Renderer:** FootnoteMarker case in DrawSpan with DOC_FOOTNOTE_CALLER color, smallFont, superscript Y offset (shared with VerseNumber).

**InputHandler:** onFootnoteTap callback + hitTestFootnoteFn member + constructor parameter. RunUnifiedFSM checks footnote hit before word tap.

**UIManager:** ShowFootnotePopup/DrawFootnotePopup/HideFootnotePopup/IsFootnotePopupActive with word-wrapped text, rounded rect, theme colors, screen-edge clamping.

**App.cpp:** hitTestFootnoteFn lambda using GetVisibleSpans (out-parameter), contentTop capture, span.height. onFootnoteTap wires to ShowFootnotePopup. Dismiss via Escape (OnDismiss popup-first) and tap-away (OnTap/OnTapEmpty top check). DrawFootnotePopup called in MainLoop after DrawToast.

**Tests:** BibleClient footnote test updated (extracts + words clean). USFM footnote test added (MAT 1 real file). 75/77 test cases pass (same 2 locale failures).

### Deferred / Future

- LayoutEngine-specific footnote test (requires constructing ChapterData with footnotes)
- Integration test with visual verification (manual: open chapter with footnotes, verify [n] markers, tap to see popup)
- Footnote scroll-to (if popup text is too tall for screen)
- Footnote text with embedded formatting (bold, italic, cross-references within footnotes)
- Emscripten clipboard fallback (`navigator.clipboard.writeText()`)

---

## Release Checklist

- [x] Bump version in `CMakeLists.txt` (1.6.3 → 1.7.0) and set suffix `-alpha`
- [x] Builds desktop (Linux, 0 warnings)
- [x] Test suite: 75/77 pass (same 2 locale failures)
- [x] Manual verification on Android (user confirmed)
- [x] Update `State.md`
- [ ] Tag release `v1.7.0-alpha`
