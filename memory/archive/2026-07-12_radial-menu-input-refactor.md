# Active (archived 2026-07-12)

> Prior active implementation: Input Refactor, Radial Menu, Android Lifecycle.

## Completed

### Input System Refactor (v1.6.1-alpha ✅)
- Unified FSM, semantic callbacks (onTap/onDragStart/etc.), TapDetector helper
- Dead event removal (RightClickEvent, ScrollStopEvent)
- FSM guard for overlays, selection drag fix, radial hitbox 1.8x scale

### Android Lifecycle (v1.6.2 ✅)
- Stop quitting on surface loss -- `ShouldQuit()` checks `destroyRequested`, not window
- Survive window-gap -- skip draw when window null, keep PollInputEvents running
- Reset FSM on resume -- `InputHandler::ResetState()` on window-restored transition
- Save/restore scroll position across OS kills

## Workstream 3: Radial Menu UX/UI 🔄

### Phase A: Sector-based Hit Detection (eliminate dead zones)

**Problem**: Circular hitboxes leave dead zones between buttons. With 2 buttons (Copy + Delete, opposite sides), the gap between hit circles is **47px** -- tapping between them dismisses the menu.

**Solution**: Replace per-button distance checks with angular sectors. Each button owns a contiguous pie-slice of the ring. Every tap within the menu boundary hits exactly one button -- no dead zones.

**Geometry (`GetSectorIndex(Vector2 pos)`)**:
```
1. Compute dist from center_ to pos
2. Outer cull:  dist > ringRadius + btnRadius x HIT_SCALE  -> return -1 (too far)
3. Inner cull:  dist < ringRadius x 0.35f                  -> return -1 (center dead zone)
4. tapAngle = atan2f(dy, dx)
5. offset = tapAngle - (startAngle - sectorAngle/2), normalized to [0, 2pi)
6. idx = floor(offset / sectorAngle)                        -> return idx
```

**What changes**:
- `RadialMenu.h` -- add `int GetSectorIndex(Vector2 pos) const;` private declaration
- `RadialMenu.cpp` -- implement helper, rewrite `UpdateHover` and `HandleClick` to use it

**What stays identical**:
- Visual layout (same ringRadius 56dp, btnRadius 18dp, same angles, same Draw)
- `LayoutButtons()`, `Show()`, `Hide()`, `Draw()` -- unchanged
- All public headers and UIManager -- unchanged
- Interface contract: `HandleClick(Vector2) -> pair<Action, int>`

### Checklist

- [x] Add `GetSectorIndex()` to RadialMenu.h
- [x] Implement `GetSectorIndex()` with 3-zone logic (inner exclusion / sector hit / outer cull)
- [x] Rewrite `UpdateHover()` to call `GetSectorIndex()`
- [x] Rewrite `HandleClick()` -- remove per-button distance loop, use sector index
- [x] Build desktop + run tests (74/76 pass expected)
- [x] Build Android APK

### Bug Fix: Highlight recolor creating duplicates

**Root cause**: `App.cpp:398` used `HighlightAtWord(result.startWord, ...)` to find an existing highlight for recoloring. A single-point lookup misses highlights whose start word is after `result.startWord`. Double-tap commits the **verse range** (e.g., words 1-10) while an existing highlight might start at word 5 -> `1 >= 5` is false -> highlight not found -> duplicate created instead of recolor.

**Fix**: Added `HighlightOverlapping(startWord, endWord, bookId, chapterNum)` that checks `h.startWord <= endWord && h.endWord >= startWord` -- any intersection. Changed App.cpp recolor lookup to use it.

- [x] Add `HighlightOverlapping()` to Highlighter.h/.cpp
- [x] Change recolor lookup in App.cpp from `HighlightAtWord(result.startWord)` -> `HighlightOverlapping(result.startWord, result.endWord)`
- [x] Build desktop + test
- [x] Build Android APK

### Debug tool: Red dot overlay

- [x] `UIManager::RecordDebugTap(Vector2 pos)` -- stores position + timestamp
- [x] `DrawDebugTap()` -- red dot at last tap, fades over 1 second
- [x] Recorded from ShowRadialMenu, HandleRadialMenuClick, and all dismiss paths

### Future Phases (after Phase A)

- **Phase B**: Press-down feedback (tint/scale on finger-down)
- **Phase C**: Show/hide animation (fade + scale from center)

### Reference files
- `src/renderer/RadialMenu.h`
- `src/renderer/RadialMenu.cpp`

## Release Checklist

- [ ] Build: desktop + Android clean
- [ ] Test: >= 70/76 pass (same locale failures)
- [ ] Manual: APK verification on device
- [ ] Update `State.md`
- [ ] Tag release

## Deferred / Backlog

- Non-contiguous verse selection
- Code quality audit beyond input system
- Copy Verse (half-implemented -- more polish needed)
