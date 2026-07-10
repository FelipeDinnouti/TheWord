# Roadmap

> Current version: v1.6.0 — Input System Refactor

## Overview

Replace the dual desktop/touch FSM + three-callback radial lifecycle with a unified
input system emitting semantic events. Consolidate 6 screens' duplicated press→drag→release
pattern into a reusable helper. Remove dead events.

12 problems (P1–P12), see `memory/INPUT_REFACTOR.md` for full analysis.

## Checklist

### ✅ Complete

- [x] **Step 1+2**: Unify `HandlePressFSM` + `HandleTouchPressFSM` → `RunUnifiedFSM()`
- [x] Replace dirty flags (`touchConsumedByRadial_`, `touchActive`, `touchScrollEmitted_`) with `didScroll_` + `prevPressed_`
- [x] **Step 3**: Replace 3 lifecycle callbacks with 7 semantic callbacks (`onTap`, `onTapEmpty`, `onDragStart`, `onDragUpdate`, `onDragEnd`, `onLongPress`, `onDismiss`)
- [x] **Step 4**: Consolidate App.cpp radial lifecycle — single `onTap` handler owns all menu logic
- [x] **Step 5**: Fix `dialogActive_` freeze — reset `pressState = Idle` + `prevPressed_ = false` before early return
- [x] **Step 6**: Verify coordinate consistency — unified FSM uses platform-appropriate source
- [x] **Step 7**: Remove dead events (`RightClickEvent`, `ScrollStopEvent`)
- [x] **Step 0**: Document Poll ordering in code comment (P12)
- [x] **Step 8**: Create `TapDetector` helper — migrated 6 screens, removed 12 fields, ~60 lines

### ⏳ Release

- [ ] Build: desktop + Android clean (desktop ✅)
- [ ] Test: 70/72 pass (same locale failures) ✅
- [ ] Update `STATE.md` ✅
- [ ] Tag `v1.6.0`

## Deferred / Backlog

- Non-contiguous verse selection
- Code quality audit beyond input system
