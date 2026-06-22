# TheWord — Sprint 3.5 Complete

## Done

### Bug Fixes (5 critical)
- **Bug 1** — Stale context menu on G/S: `hideContextMenu()` before toggle
- **Bug 2** — Outside-click leaks into selection: early return after context menu click
- **Bug 3** — Dead `dismissActiveDialog()`: unified Escape handler now calls it
- **Bug 4** — Context menu clicks leak through: fixed by Bug 2's early return
- **Bug 5** — Highlight orphaning on version switch: `provider_name` column, Highlighter filters by provider

### UX Polish
- Labels: `"Src:"`→`"Source:"`, `"Clr:"`→`"Color:"`
- Close buttons ("X") on go-to and settings dialogs
- Font buttons gray out at min(12)/max(36)
- Settings panel height adjusts dynamically when Source row hidden
- Context menu flips left on right-edge overflow
- FPS counter gated behind `#ifndef NDEBUG`

### Clean Code Refactors
- Magic numbers → named constants (`BACKDROP_ALPHA`, `CLOSE_SIZE`, `CULL_MARGIN`, `MIN_SCROLLBAR_HEIGHT`, `KEYBOARD_SCROLL_FACTOR`, `LABEL_SWATCH_GAP`, `GO_TO_DIALOG_HEIGHT`)
- Variable renames: `sw/sh`→`screenW/H`, `dx/dy`→`dlgX/Y`, `sx/sy`→`panelX/Y`, `m`→`mousePos`, `csx`→`swatchX`, `c`(char)→`ch`
- Extracted helpers: `drawBackdrop()`, `applyFontSize(float)`, `getGoToDialogRect()`, `getSettingsPanelRect()`, `getCloseButtonRect()`
- Fixed variable shadowing in `parseGoToInput` inner loop

## Pending Function Rewrites

These functions are too large or have too many responsibilities. They should be split in a future sprint.

### 1. `InputHandler::handleInput()` — 159 lines
**Problem**: Handles 6+ responsibilities: dialog routing, keyboard shortcuts, scroll, right-click, press FSM, window resize.
**Plan**: Extract into sub-methods:
- `handleDialogInput()` — routes to active dialog handler
- `handleKeyboardShortcuts()` — G/S keys
- `handleScroll()` — wheel + arrow keys + friction
- `handleRightClick()` — context menu on highlighted word
- `handlePressFSM()` — the Idle/Pending/Dragging/LongPress state machine
- `handleWindowResize()` — reflow on resize

### 2. `UIManager::handleSettingsClick()` — 80 lines
**Problem**: Handles 5 different click targets in one function (close, font−, font+, version toggle, color swatch).
**Plan**: Extract per-target handlers:
- `handleFontMinusClick()`, `handleFontPlusClick()`
- `handleVersionToggleClick(Vector2)`
- `handleColorSwatchClick(Vector2)`

### 3. `UIManager::drawSettingsPanel()` — 57 lines
**Problem**: Draws 4 distinct sections (title+close, font row, source row, color row).
**Plan**: Extract per-section draw methods:
- `drawSettingsTitleAndClose(Rectangle panel)`
- `drawSettingsFontRow(Rectangle panel, float rowY)`
- `drawSettingsSourceRow(Rectangle panel, float rowY)`
- `drawSettingsColorRow(Rectangle panel, float rowY)`

### 4. `UIManager::drawGoToDialog()` — 38 lines
**Problem**: Draws backdrop + title + close button + input box + blinking cursor + suggestion list.
**Plan**: Extract `drawGoToSuggestions(Rectangle dlg)` and `drawGoToInput(Rectangle dlg)`.

### 5. `UIManager::parseGoToInput()` — 38 lines
**Problem**: Contains 3 distinct parsing strategies (book.code+chapter, fullName+chapter, space-separated) in one function.
**Plan**: Split into testable helpers returning `std::optional<std::string>`:
- `tryParseBookDotChapter(const std::string& input)`
- `tryParseFullNameThenChapter(const std::string& input)`
- `tryParseSpaceSeparated(const std::string& input)`

### 6. `UIManager::handleGoToKeyboardInput()` — 38 lines
**Problem**: Handles 6 different keys: char input, Backspace, Up, Down, Tab, Enter.
**Plan**: Each key could be a small handler, or the method could delegate to per-key lambdas.

## DB Schema
- `highlights` table now has `provider_name TEXT NOT NULL DEFAULT ''`
- Migration via `ALTER TABLE` on startup (error-ignored if exists)
