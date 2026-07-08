# Roadmap: v1.6.0-alpha.1

> Theme: Radial Action Menu (replaces ContextMenu) + Copy Verse + Double-click
> Status: Phases 1-2 complete, ready for Phase 3 release

## Overview

Replace the old right-click context menu with a unified radial action menu that
appears after any text selection (drag, tap, or double-click). The same menu
handles highlight (pick a color), copy, and delete actions.

```
Select text (tap/drag/double-click)
  → radial menu centered on last pointer position
       ├─ 5 color circles → highlight selection with that color
       ├─ C (Copy)       → copy selected word text to clipboard
       └─ X (Delete)     → remove overlapping highlights
  → tap outside / Escape → dismiss, clear selection
```

## Phase 1 — Radial Menu + New Selection Flow

### Files Created
| File | Purpose |
|------|---------|
| `src/renderer/RadialMenu.h` | 7-circle radial menu widget |
| `src/renderer/RadialMenu.cpp` | Layout, draw, hit-test |

### Files Modified
| File | Change |
|------|--------|
| `Highlighter.h/cpp` | EndSelection() records committed range, doesn't auto-create. Add CreateHighlight(), ClearCommittedSelection(), HasCommittedSelection() |
| `UIManager.h/cpp` | Replace ContextMenu with RadialMenu. ShowRadialMenu(), DrawRadialMenu(), HandleRadialMenuClick() returns action struct |
| `InputHandler.h/cpp` | Tap/drag show radial menu instead of auto-highlight. Add double-click detection + RadialMenuCallbacks. Remove old RightClickEvent emission |
| `App.cpp` | Wire radial lifecycle (show on selection end, process Copy/Delete/Highlight) |
| `ReaderScreen.cpp` | Persist selection tint after release. Lighter shade for verse-exact selections |
| `CMakeLists.txt` | RadialMenu.cpp added, ContextMenu.cpp/h removed |

### Files Deleted
| File | Reason |
|------|--------|
| `src/renderer/ContextMenu.h` | Replaced by RadialMenu |
| `src/renderer/ContextMenu.cpp` | Replaced by RadialMenu |

### Checklist
- [x] Plan written
- [x] Create RadialMenu.h — class with Show/Hide/Draw/HandleClick, Button struct for 7 circles
- [x] Create RadialMenu.cpp — LayoutButtons (evenly spaced around center), draw circles, hit-test
- [x] Modify Highlighter.h — add committedStart/End, HasCommittedSelection(), ClearCommittedSelection(), CreateHighlight(start,end,typeId)
- [x] Modify Highlighter.cpp — EndSelection() records range, doesn't create. CreateHighlight() extracted. ClearCommittedSelection() added
- [x] Modify UIManager.h — unique_ptr<RadialMenu>, RadialMenuActionResult struct, ShowRadialMenu(), HandleRadialMenuClick()
- [x] Modify UIManager.cpp — wire RadialMenu, handle Highlight action
- [x] Modify InputHandler.h — RadialMenuClickCallback, RadialMenuShowCallback, double-click tracking
- [x] Modify InputHandler.cpp — tap no longer auto-highlights, drag-release shows radial menu, double-click detection, right-click select+show, mobile tap any word shows menu
- [x] Modify App.cpp — radial lifecycle (show/handle), copy action (AssembleSelectedText + SetClipboard), delete action (remove overlapping highlights), double-click verse expansion (FindVerseRange)
- [x] Modify ReaderScreen.cpp — draw committed selection tint + lighter shade for verse-exact
- [x] Update CMakeLists.txt — +RadialMenu, -ContextMenu
- [x] Delete ContextMenu.h/cpp
- [x] Build + verify (70/72 tests pass, same 2 locale failures)
- [x] Update tests — replace all OnSelection(End) with CreateHighlight calls

## Phase 2 — Copy, Delete, Double-Click
All Phase 2 items implemented as part of Phase 1.

### Checklist
- [x] RadialMenu: Copy ("C") and Delete ("X") buttons in the ring
- [x] App.cpp: Copy action → AssembleSelectedText, platform::SetClipboard
- [x] App.cpp: Delete action → find overlapping highlights, remove each
- [x] InputHandler: double-click detection (lastClickTime + lastClickWord, DOUBLE_CLICK_TIME)
- [x] App.cpp: double-click → FindVerseRange, expand selection to whole verse
- [x] ReaderScreen: lighter blue tint when selection exactly matches verse boundary
- [x] Build + 70/72 tests passing

## Phase 3 — Release
- [ ] Full test suite passes (72/72, or 70/72 locale pending)
- [ ] Desktop build clean
- [ ] Android APK build
- [ ] Tag `v1.6.0-alpha.1`
- [ ] Update `STATE.md` with final state
