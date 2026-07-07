# Roadmap: v1.6.0-alpha.1

> Theme: Copy Verse + Code Quality
> Status: Planning complete, ready to implement

## Overview

Two features, no dependencies between them:

| # | Feature | Type | Est. files touched |
|---|---------|------|-------------------|
| 1 | `unique_ptr<ContextMenu>` refactor | refactor | 2 |
| 2 | Copy Verse | feat | 5-6 |

## Checklist

### Phase 0: Version Bump & Doc Updates
- [x] Bump `CMakeLists.txt` → `VERSION 1.6.0`, suffix `-alpha.1`
- [x] `reconfigure` — `rm -rf build && cmake -B build ...`
- [x] Update `Release Plan.md` — move 1.5.0 to past, add 1.6.0 section
- [x] Update `Progress Tracking.md` — mark 1.5.0-alpha.x done, add 1.6.0 section
- [x] Update `Agent Workflow.md` — reference `memory/` for state + roadmap

### Phase 1: unique_ptr<ContextMenu> Refactor
- [ ] `UIManager.h` — change `ContextMenu*` → `std::unique_ptr<ContextMenu>`
- [ ] `UIManager.cpp` — `new` → `std::make_unique`, remove `delete`
- [ ] Build + verify no regressions
- [ ] Run tests (should still be 70/72)

### Phase 2: Copy Verse
- [ ] **Research / design**: Determine exact context menu expansion approach
- [ ] **ContextMenu**: Add "Copy" button alongside existing highlight controls; support showing on non-highlighted words
- [ ] **UIManager**: Add copy callback or extend ShowContextMenu to include copy action
- [ ] **App.cpp**: Modify right-click handler to show context menu even when word is NOT highlighted; handle copy action (assemble verse text from `ChapterData::words`, call `platform::SetClipboard`)
- [ ] **Verse text assembly**: Helper to collect all words with matching `verseId` → format as "Book Chapter:Verse text"
- [ ] **Mobile long-press**: Ensure long-press on non-highlighted text also triggers copy context menu
- [ ] Build + run tests
- [ ] Manual verification (desktop: right-click a verse → copy → paste)

### Phase 3: Release
- [ ] Full test suite passes (72/72, or 70/72 locale pending)
- [ ] Desktop build clean
- [ ] Tag `v1.6.0-alpha.1`
- [ ] Update `STATE.md` with final state
