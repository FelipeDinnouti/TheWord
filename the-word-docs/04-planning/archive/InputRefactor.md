# Input System Refactor: Unified FSM → Semantic Events

> Status: **Design phase** — no implementation yet.
> Scope: `InputHandler.h/cpp` + `App.cpp` (radial lifecycle), plus opportunistic cleanup in `event/Events.h` and a new `ui/TapDetector.h`.
> Goal: Eliminate 7+4 structural problems in the click/FSM architecture.

---

## Part 1: Full Architecture of Current Input System

### 1.1 System Overview (16 files, 4 groups)

```
┌──────────────────────────────────────────────────────────────────┐
│ GROUP A: Raw Input FSM (1 file)                                  │
│  input/InputHandler.h/cpp — 389 lines                            │
│  Central FSM: touch + mouse + scroll + pinch + keyboard          │
│  3 callbacks to App, EventBus events to subsystems                │
│  15 state flags                                                   │
└──────────────────────────────────────────────────────────────────┘
         │
         │ Poll(deltaTime, navStack, 3 callbacks) called every frame
         ▼
┌──────────────────────────────────────────────────────────────────┐
│ GROUP B: Screen-level HandleInput (7 files, duplicated pattern)  │
│  ui/ReaderScreen.cpp       — bottom bar clicks                   │
│  ui/CenterMenu.cpp         — press→drag→release on menu items    │
│  ui/BookListScreen.cpp     — press→drag→release on books         │
│  ui/HighlightBrowserScreen.cpp — press→drag→release on items     │
│  ui/SettingsScreen.cpp     — press→drag→release on settings      │
│  ui/CreditsOverlay.cpp     — press→drag→release, dismiss outside │
│  ui/ChapterGridScreen.cpp  — press→drag→release on grid cells    │
│                                                                    │
│  Each screen has its own pressStartPos_ / hasPendingPress_ fields │
│  6 screens identical pattern, 1 screen (ReaderScreen) bottom-bar │
└──────────────────────────────────────────────────────────────────┘
         │
         │ navStack->HandleInput(deltaTime) — delegates to active screen
         ▼
┌──────────────────────────────────────────────────────────────────┐
│ GROUP C: Shared UI Components (1 file)                           │
│  ui/components.cpp — DrawButton/DrawTextItem/DrawToggle/          │
│                      DrawColorSwatch                             │
│  Each calls GetMousePosition() + IsMouseButtonPressed/Released   │
│  Used by Group B screens for uniform visual interaction           │
└──────────────────────────────────────────────────────────────────┘
         │
         │ Used by screens for drawing/interaction
         ▼
┌──────────────────────────────────────────────────────────────────┐
│ GROUP D: App Orchestration (1 file)                              │
│  app/App.cpp — wires 3 radial lifecycle lambdas                  │
│  app/App.h   — accumStartWord_, accumEndWord_                    │
│  Owns the accumulator state, manages it across 5 sites            │
└──────────────────────────────────────────────────────────────────┘
```

### 1.2 Per-Frame Poll Flow (InputHandler.cpp)

```
App::Run() [line 454]:
  inputHandler_->Poll(deltaTime, navStack_,
                      radialClickHandler,        // 3 callbacks from App
                      radialDismiss,
                      radialShowHandler)

InputHandler::Poll() [line 31]:
  │
  ├─ 1. showRadialCallback_ = radialShowHandler (save for FSM to call later)
  │
  ├─ 2. Escape key → contextDismissHandler() [line 38]
  │     └─ If menu active → HideRadialMenu(), clear accumulator, return
  │
  ├─ 3. LEFT MOUSE PRESSED → radialClickHandler(GetMousePosition()) [line 41-50]
  │     └─ This fires on TOUCH-DOWN (on Android, touch === left mouse)
  │     └─ UIManager::HandleRadialMenuClick(pos)
  │           └─ RadialMenu::HandleClick(pos) → Action + colorIndex
  │     └─ If consumed=true:
  │           touchConsumedByRadial_ = true
  │           touchActive = false           (!! side effect !!)
  │           touchLaunchVelocity_ = 0.0f   (!! side effect !!)
  │           return (skip rest)
  │     └─ If consumed=false:
  │           (menu not active OR tap outside buttons → let FSM handle)
  │
  ├─ 4. navStack->HandleInput(deltaTime) [line 53]
  │     └─ Delegates to active Screen's HandleInput()
  │     └─ Screens call IsMouseButtonPressed/Released directly
  │     └─ If screen returns true → consume, skip rest
  │
  ├─ 5. Escape key (not consumed by radial or screen) [line 55-57]
  │     └─ Emit KeyEvent{ESCAPE}
  │
  ├─ 6. dialogActive_ guard [line 60-65]
  │     └─ If dialog open → skip all pointer/key processing
  │     └─ (only G/S/A hotkeys pass through for dialog toggle)
  │
  ├─ 7. Touch platform? [line 67-75]
  │     ├─ HandlePinch()           [line 362] → FontSizeEvent
  │     ├─ HandleTouchScroll()     [line 231] → ScrollEvent + momentum
  │     └─ HandleTouchPressFSM()   [line 267] → Tap/Drag/LongPress FSM
  │
  └─ 8. Desktop platform? [line 71-74]
        ├─ HandleScroll()          [line 96]  → Mouse wheel + arrow keys
        ├─ HandleRightClick()      [line 216] → SelectionEvent + radial show
        └─ HandlePressFSM()        [line 113] → Tap/Drag/LongPress FSM
```

### 1.3 The Three-Callback Contract (App.cpp)

#### Callback 1: `radialClickHandler` [App.cpp:348-397]
```
Called: InputHandler line 41 — on IsMouseButtonPressed (TOUCH-DOWN)
Signature: bool(Vector2 pos)
Logic:
  HandleRadialMenuClick(pos)
  → If button hit (Highlight/Copy/Delete):
        UIManager handles action, HIDES menu internally
        accumStartWord_/EndWord_ cleared
        Return true (consumed → touchConsumedByRadial_=true, FSM blocked)
  → If missed buttons (Action::None):
        HitTestWord at pos
        → word found + accumStartWord_>=0 (expansion mode):
              Return false (menu stays, FSM runs, showRadialCallback_ will expand)
        → word found + accumStartWord_<0:
              HIDE menu, clear accumulator
              Return false (FSM runs, showRadialCallback_ may re-show)
        → no word (empty space):
              HIDE menu, clear accumulator
              Return false
Returns: bool — consumed (FSM blocked) or not (FSM runs)
```

#### Callback 2: `radialShowHandler` [App.cpp:409-445]
```
Called: InputHandler at 6 call sites:
  - HandlePressFSM: tap on highlighted/double-click word     [line 143-144]
  - HandlePressFSM: tap on any word                          [line 145-146]
  - HandlePressFSM: long-press on highlighted word           [line 157]
  - HandleTouchPressFSM: tap on highlighted/double-click word [line 308-309]
  - HandleTouchPressFSM: tap on any word                     [line 310-311]
  - HandleTouchPressFSM: release from selecting state         [line 340]
  ...all on TOUCH-UP (release)
Signature: void(int startWord, int endWord, Vector2 position, bool selectFullVerse)
Logic:
  If navStack not on root → return (P2 guard)
  If no chapter data → return

  If startWord == endWord (single word selected):
    → If word is highlighted:
        Use full highlight range (start..end), exit expansion
        accumStartWord_ = -1, accumEndWord_ = -1
    → If selectFullVerse (double-click):
        FindVerseRange at word → vStart..vEnd
        accumStartWord_ = min(existing, vStart)
        accumEndWord_ = max(existing, vEnd)
    → If accumStartWord_ >= 0 (expansion mode from previous tap):
        FindVerseRange at word → vStart..vEnd
        accumStartWord_ = min(accumStartWord_, vStart)
        accumEndWord_ = max(accumEndWord_, vEnd)
    → Else:
        return; // no-op — single unhighlighted word, no expansion
  Else (startWord != endWord, drag selection):
    accumStartWord_ = -1, accumEndWord_ = -1 (exit expansion)

  Finally: ShowRadialMenu(position, startWord, endWord)
```

#### Callback 3: `radialDismiss` [App.cpp:399-407]
```
Called: InputHandler line 38 — on ESCAPE key press
Signature: bool()
Logic:
  If radial menu active → HideRadialMenu(), clear accumulator, return true
  Else → return false (Escape falls through to KeyEvent)
```

#### Accumulator State (App.h:65-66)
```
int accumStartWord_ = -1;  // first word of accumulated selection
int accumEndWord_ = -1;    // last word of accumulated selection

Written at 2 sites (both in radialShowHandler):
  [line 425-428] double-click: set to verse range
  [line 433-434] expansion tap: min/max with new verse range

Cleared at 4 sites:
  [line 362-363] radialClickHandler: true dismiss (empty space or word without expansion)
  [line 368-369] radialClickHandler: action (Copy/Delete/Highlight)
  [line 402-403] radialDismiss: Escape key
  [line 328-329] ChapterLoadedEvent: chapter change
```

### 1.4 The 15 State Flags — Complete Inventory

All flags are members of `InputHandler` (`input/InputHandler.h:37-62`).

```
╔══════════════════════╤══════════╤══════════════════════════════════╗
║ Flag                 │ Type     │ Purpose                          ║
╠══════════════════════╪══════════╪══════════════════════════════════╣
║ 1. pressState        │ enum(5)  │ FSM state: Idle/Pending/         ║
║                      │          │ Dragging/LongPress/Selecting     ║
╟──────────────────────┼──────────┼──────────────────────────────────╢
║ 2. pressStartTime    │ double   │ When press started (long-press)  ║
╟──────────────────────┼──────────┼──────────────────────────────────╢
║ 3. pressStartPos     │ Vector2  │ Where press started (drag)       ║
╟──────────────────────┼──────────┼──────────────────────────────────╢
║ 4. pressStartWord    │ int      │ Word under press start           ║
╟──────────────────────┼──────────┼──────────────────────────────────╢
║ 5. selectStartWord   │ int      │ Anchor word for long-press drag  ║
╟──────────────────────┼──────────┼──────────────────────────────────╢
║ 6. touchActive       │ bool     │ Touch scroll gesture active      ║
╟──────────────────────┼──────────┼──────────────────────────────────╢
║ 7. touchLastY        │ float    │ Previous Y for scroll delta      ║
╟──────────────────────┼──────────┼──────────────────────────────────╢
║ 8. touchLaunchVel_   │ float    │ Scroll momentum velocity         ║
╟──────────────────────┼──────────┼──────────────────────────────────╢
║ 9. lastPinchDist     │ float    │ Pinch span (2-finger font size)  ║
╟──────────────────────┼──────────┼──────────────────────────────────╢
║10. touchScrollEmitt_ │ bool     │ "Scroll started" — blocks tap    ║
╟──────────────────────┼──────────┼──────────────────────────────────╢
║11. lastTouchPos_     │ Vector2  │ Last touch position (menu pos)   ║
╟──────────────────────┼──────────┼──────────────────────────────────╢
║12. lastClickTime_    │ double   │ Previous tap time (double-tap)   ║
╟──────────────────────┼──────────┼──────────────────────────────────╢
║13. lastClickWord_    │ int      │ Previous tap word (double-tap)   ║
╟──────────────────────┼──────────┼──────────────────────────────────╢
║14. touchConsumedBy_  │ bool     │ "Radial handled this touch"      ║
║    Radial_           │          │ → block FSM re-entry              ║
╟──────────────────────┼──────────┼──────────────────────────────────╢
║15. dialogActive_     │ bool     │ "Dialog open" → skip all input   ║
╚══════════════════════╧══════════╧══════════════════════════════════╝
```

#### Flag Interaction Map

```
TOUCH-DOWN frame:
  HandleTouchScroll [line 237-243]:
    touchActive=false → touchActive=true, slopAccumulator=0, return (early)

  HandleTouchPressFSM [line 276-291]:
    pressState=Idle
    if touchCount==1 && !touchScrollEmitted_ && !touchConsumedByRadial_:
      pressStartTime=now, pressStartPos=pos, pressStartWord=hitTest
      pressState=Pending
    if touchCount==0:
      touchScrollEmitted_=false, touchConsumedByRadial_=false

SUBSEQUENT FRAMES (finger still down):
  HandleTouchScroll [line 245-264]:
    touchActive already true
    deltaY = pos.y - touchLastY
    touchLastY = pos.y
    touchLaunchVelocity_ = -deltaY / dt
    slopAccumulator += deltaY
    if abs(slopAccumulator) < TOUCH_SLOP → return (no scroll yet)
    touchScrollEmitted_ = true
    emit ScrollEvent

  HandleTouchPressFSM:Pending [line 293-332]:
    if touchCount==0: (release)
      if pressStartWord>=0 && !touchScrollEmitted_ → TAP
      touchScrollEmitted_=false, pressState=Idle
    elif time > LONG_PRESS_TIME:
      if pressStartWord>=0 → selectStartWord=pressStartWord, pressState=Selecting
      else → pressState=Idle
    else:
      dy = pos.y - pressStartPos.y
      if abs(dy) > LONG_PRESS_MOVE_THRESHOLD || touchScrollEmitted_:
        pressState=Idle  ← ABORTED TAP

RELEASE frame:
  HandleTouchScroll [line 261-264]:
    touchActive=false
    emit ScrollEvent{0, false, touchLaunchVelocity_}

  HandleTouchPressFSM:Idle [line 287-290]:
    touchScrollEmitted_=false, touchConsumedByRadial_=false
```

### 1.5 The 14 Behavioral Differences Between Desktop & Touch FSM

| Aspect | HandlePressFSM (desktop) | HandleTouchPressFSM (touch) |
|--------|--------------------------|------------------------------|
| Entry into Pending | `IsMouseButtonPressed(MOUSE_LEFT_BUTTON)` | `touchCount==1 && !touchScrollEmitted_ && !touchConsumedByRadial_` |
| Release detection | `!IsMouseButtonDown(MOUSE_LEFT_BUTTON)` | `touchCount == 0` |
| Coordinate source | `GetMousePosition()` everywhere | `GetTouchPosition(0)` at entry, `lastTouchPos_` for callbacks |
| Drag detection axes | Both dx AND dy (full 2D) | Only dy (vertical) |
| Drag transition | `Pending → Dragging` (separate state) | `Pending → Idle` (aborts, no drag feature) |
| Long-press trigger | Only if `isHighlightedFn()` returns true | Always if `pressStartWord >= 0` |
| Long-press transition | `Pending → LongPress` | `Pending → Selecting` |
| Selection tracking | `pressStartWord` fixed, `endWord` per-frame hitTest | `selectStartWord` anchor, `pressStartWord` mutates on Update |
| Guard flags | None | `touchScrollEmitted_`, `touchConsumedByRadial_` |
| Long-press release | `LongPress → Idle`, no action | `Selecting → Idle`, emits `SelectionEvent::End` + `FinishSelection` |
| Tap on non-highlighted | Always calls `showRadialCallback_` | Always calls `showRadialCallback_` (same) |
| Double-click guard | Inline (lines 131-136) | Inline (lines 297-301) — identical copy |
| Release of selected drag | Emits `SelectionEvent::End` + `FinishSelection` | Selecting state does long-press-hold selection (no desktop-style drag) |
| Right mouse button | `HandleRightClick()` exists | No touch equivalent (menu shown by other means) |

### 1.6 Touch Handling — All Code Paths

```
GetTouchPointCount() called in 4 locations:
  ├─ HandleTouchScroll      [InputHandler.cpp:231]
  │   → slopAccumulator, touchLastY, touchLaunchVelocity_
  │   → touchScrollEmitted_ flag
  │   → Emits ScrollEvent{delta, direct=true, velocity}
  │
  ├─ HandleTouchPressFSM    [InputHandler.cpp:267]
  │   → FSM: Idle → Pending → Selecting
  │   → On touchCount==0 (release): tap detection
  │   → Guards: !touchScrollEmitted_, !touchConsumedByRadial_
  │   → Uses lastTouchPos_ for callback position
  │
  ├─ HandlePinch            [InputHandler.cpp:362]
  │   → 2+ fingers → FontSizeEvent
  │   → Side effects: touchActive=false, pressState=Idle
  │
  └─ (Accidental) Poll path [InputHandler.cpp:42]
      → radialClickHandler(GetMousePosition())
      → NOT GetTouchPosition(0)!
      → fires on touch-down via IsMouseButtonPressed
      → sets touchConsumedByRadial_ if consumed

GetTouchPosition(0) called in 3 locations:
  ├─ HandleTouchScroll      [line 236]
  ├─ HandleTouchPressFSM    [line 271]
  └─ HandlePinch            [lines 365-366] (both finger 0 and 1)
```

### 1.7 Screen-Level Input Duplication (6 files)

Every screen except ReaderScreen implements this identical pattern:

```cpp
// In .h:
Vector2 pressStartPos_;
bool hasPendingPress_;

// In .cpp HandleInput():
if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
    pressStartPos_ = GetMousePosition();
    hasPendingPress_ = true;
}

if (hasPendingPress_ && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
    hasPendingPress_ = false;
    Vector2 mousePos = GetMousePosition();
    float dx = mousePos.x - pressStartPos_.x;
    float dy = mousePos.y - pressStartPos_.y;
    if (dx * dx + dy * dy > uiScale_.dp(10) * uiScale_.dp(10))
        return true; // was a drag, not a tap
    // ... hit test items at mousePos ...
}
```

Screens using this: `CenterMenu`, `BookListScreen`, `HighlightBrowserScreen`, `SettingsScreen`, `CreditsOverlay`, `ChapterGridScreen`.

Each declares `pressStartPos_` and `hasPendingPress_` in its own `.h` file. The threshold (10dp) is hardcoded in each. If we consolidate into a reusable `TapDetector`, we eliminate ~60 lines of duplication + 12 member fields across 6 files.

---

## Part 2: Problem Index

### P1: Timing Mismatch — Touch-Down vs Touch-Up Race
- **Location**: `InputHandler.cpp:41-50` (touch-down handler) vs `:293-315` (touch-up FSM)
- **Description**: `radialClickHandler` fires on `IsMouseButtonPressed(MOUSE_LEFT_BUTTON)` (touch-down). The FSM processes taps on `touchCount==0` (touch-up). These are separated by many frames. `radialClickHandler` may hide the menu on touch-down, then scroll resets the FSM before touch-up, so the menu never re-appears.
- **Root cause**: The callback interface is designed for desktop mouse semantics (press+release = atomic "click") but invoked on touch platforms where press and release are temporally separated.
- **Symptoms**: Scroll dismisses the menu permanently. Menu flickers vanish-then-reappear on tap.

### P2: Near-Identical Duplicated FSM
- **Location**: `InputHandler.cpp:113-214` vs `:267-360`
- **Description**: Two complete FSM implementations with 14 behavioral differences (see §1.5). Every feature/bugfix applied twice.
- **Root cause**: Touch and desktop were split early and never re-integrated. Platform-specific behaviors (guards, coordinate sources) became global divergences.
- **Symptoms**: Subtle behavioral differences between platforms (e.g., desktop drag uses 2D, touch drag uses 1D — likely a latent bug).

### P3: Flag Proliferation & Fragile Interactions
- **Location**: `InputHandler.h:37-62` (15 state variables)
- **Description**: Flags set in one subsystem and cleared in another create hidden couplings:
  - `touchConsumedByRadial_` set in `Poll()` but cleared in `HandleTouchPressFSM::Idle`
  - `touchActive` cleared from 3 locations (radial click, touch-up, pinch)
  - `touchScrollEmitted_` and `touchConsumedByRadial_` both cleared together — can't distinguish after the fact
  - `slopAccumulator` not reset on radial dismiss path
- **Root cause**: No encapsulation boundary between subsystems sharing the same InputHandler instance.
- **Symptoms**: Edge-case bugs that are hard to reproduce (ghost scrolls, missed taps after radial action).

### P4: Callback Split — Distributed State Machine
- **Location**: `App.cpp:348-445` + `InputHandler.cpp` at 9 call sites
- **Description**: Radial menu lifecycle split across 3 lambdas in App, called from FSM in InputHandler. Accumulator managed across 5 sites in 2 files. Neither side has complete state knowledge.
- **Root cause**: No single owner of the "should I show/hide/update the radial menu?" decision.
- **Symptoms**: Accumulator can be cleared by `radialClickHandler` (touch-down) then re-read by `showRadialCallback_` (touch-up) with stale value. Hard to reason about.

### P5: Touch-Down Decision on Incomplete Intent
- **Location**: `UIManager.cpp:72-74` (Action::None → HideRadialMenu)
- **Description**: The radial menu's hit-test runs on touch-down, but user intent (tap vs scroll vs long-press) is only known on release. An Action::None + expansion mode check hides the menu prematurely.
- **Root cause**: `HandleRadialMenuClick` is designed for instantaneous click events (desktop mouse), not touch-down events that may become scrolls.
- **Symptoms**: Two-frame menu flicker.

### P6: Coordinate Mismatch — GetMousePosition vs GetTouchPosition
- **Location**: `InputHandler.cpp:42` (mouse) vs `:236,271` (touch)
- **Description**: `radialClickHandler` uses `GetMousePosition()`, but `HandleTouchScroll` and `HandleTouchPressFSM` use `GetTouchPosition(0)`. Raylib aliases these on Android currently, but this is not API-guaranteed.
- **Root cause**: Different paths in the same `Poll()` frame query different input sources.
- **Symptoms**: Off-by-N-pixel hit-test errors on Android if coordinates ever diverge.

### P7: dialogActive_ Freezes Dragging State
- **Location**: `InputHandler.cpp:60-65` (early return) + `:181-202` (Dragging state)
- **Description**: When `dialogActive_` becomes true, `Poll()` returns early, skipping the FSM. Desktop `Dragging` state never sees `IsMouseButtonReleased` (a one-shot event consumed while dialog was active). `IsMouseButtonDown` becomes false, but no state transition handles this.
- **Root cause**: Early return bypasses FSM release detection. Touch platform is immune because it uses `touchCount==0` (state, not event).
- **Symptoms**: Selection highlight permanently stuck. Requires app restart.

### P8: 6-Screen HandleInput Duplication (NEW)
- **Location**: `ui/CenterMenu.cpp`, `ui/BookListScreen.cpp`, `ui/HighlightBrowserScreen.cpp`, `ui/SettingsScreen.cpp`, `ui/CreditsOverlay.cpp`, `ui/ChapterGridScreen.cpp`
- **Description**: Each screen reimplements press→drag→release with identical logic. Same 10px slop threshold, same `pressStartPos_`/`hasPendingPress_` fields. ~60 lines × 6 files = ~360 lines of duplication.
- **Root cause**: No shared abstraction for "detect a tap on a UI element."
- **Symptoms**: If we fix the global slop threshold, we'd need to update it in 6 places. If we add a new screen, we copy-paste the same pattern.

### P9: Dead Events in EventBus (NEW)
- **Location**: `event/Events.h:35-36`
- **Description**: `RightClickEvent` and `ScrollStopEvent` are defined but never emitted or listened to. Leftovers from the old ContextMenu/ActionMenu era.
- **Root cause**: Events accumulated during development, never pruned when the features they supported were replaced.
- **Symptoms**: Dead code in the bus. Confuses new readers. Small binary size bloat.

### P10: Unclear Semantic Event Transport (NEW — Design Choice)
- **Location**: The refactor proposes 7 new callbacks on InputHandler. But we already have an EventBus.
- **Description**: If we use direct callbacks, the coupling is tight but simple. If we use EventBus events (`TapEvent`, `DragEndEvent`, etc.), any module can observe input without modifying InputHandler. Trade-offs exist.
- **Root cause**: No prior design decision documented.
- **Symptom**: Either we commit to callbacks and may want events later, or we use events and add complexity now.

### P11: Selecting vs LongPress Semantic Divergence (NEW)
- **Location**: `InputHandler.cpp:150-158` (desktop) vs `:316-325` (touch)
- **Description**: Desktop `LongPress` only fires for highlighted words and does nothing on release. Touch `Selecting` fires for any word and emits `SelectionEvent::End` + `FinishSelection` on release. These are two different features sharing the same timer.
- **Root cause**: The unified FSM must express "long-press on any word → start drag selection" AND "long-press on highlighted word → show menu" as two different behaviors triggered by the same timing event. The current code expresses this as platform-specific branches, but the real dimension is "what type of word was pressed," not "what platform."
- **Symptom**: When unifying the FSM, we need a "long-press behavior" abstraction, not just an `if(hasTouch)` switch.

### P12: Poll Ordering — Screens Can Steal Events from FSM (NEW)
- **Location**: `InputHandler.cpp:53` (navStack→HandleInput runs before FSM)
- **Description**: Screens' `HandleInput` runs between the radial click handler and the FSM. A screen calling `IsMouseButtonPressed` can consume the event before the FSM sees it. Currently works only because screens handle UI (not text), but undocumented.
- **Root cause**: No explicit layering contract between "UI input" (screens) and "text input" (FSM).
- **Symptom**: If a screen were to intercept a word-area press, the FSM would never process it.

---

## Part 3: Proposed Architecture

### 3.1 High-Level Design

```
InputHandler                              App (orchestrator)
(pure input translator)                   (owns all state + decisions)
┌──────────────────────┐                  ┌──────────────────────────┐
│  Per frame:          │   onTap()        │  Tap handler:            │
│  - Poll input state  │ ◄─────────────── │  One unified entry point │
│  - Run unified FSM   │                  │  for all word/empty taps │
│  - Emit events       │   onTapEmpty()   │                          │
│                      │ ◄─────────────── │  - Radial menu active?   │
│  Unified FSM:        │                  │    → hit button? action  │
│  Idle ──(press)──→   │   onDragStart()  │    → expansion tap? keep │
│    Pending           │ ◄─────────────── │    → single word? show   │
│      │(tap)──→ Idle  │                  │    → empty space? dismiss│
│      │(drag)──→ Drag │   onDragUpdate() │  - No menu active?       │
│      │(lp)──→ LPress │ ◄─────────────── │    → double/highlight?   │
│  Drag ──(release)──→ │                  │    → show menu           │
│    Idle              │   onDragEnd()    │                          │
│  LPress─(release)──→ │ ◄─────────────── │  Accumulator logic:      │
│    Idle              │                  │  In one place, not       │
│                      │   onLongPress()  │  split across 3 fns      │
│  No radial awareness │ ◄─────────────── │                          │
│  No accumulator      │                  │  onScroll() → ScrollEvent│
│  No selection logic  │   onScroll()     │  (separate subsystem)    │
│                      │ ◄─────────────── │                          │
└──────────────────────┘                  └──────────────────────────┘
```

### 3.2 Key Changes

#### Change 1: Unified FSM (replaces HandlePressFSM + HandleTouchPressFSM)

```cpp
void InputHandler::RunUnifiedFSM() {
    // Abstract input predicates — chose per-platform at top of Poll()
    bool isPressed = platform::HasTouchInput()
        ? (GetTouchPointCount() >= 1)
        : IsMouseButtonDown(MOUSE_LEFT_BUTTON);
    bool wasPressed = prevPressed_;
    bool justPressed = isPressed && !wasPressed;
    bool justReleased = !isPressed && wasPressed;
    Vector2 pos = platform::HasTouchInput()
        ? GetTouchPosition(0)
        : GetMousePosition();
    prevPressed_ = isPressed;

    switch (pressState) {
        case Idle:
            if (justPressed) {
                pressStartTime = GetTime();
                pressStartPos = pos;
                pressStartWord = hitTestFn ? hitTestFn(pos.x, pos.y) : -1;
                didScroll_ = false;
                pressState = Pending;
            }
            break;

        case Pending:
            if (justReleased) {
                if (pressStartWord >= 0 && !didScroll_) {
                    // It's a tap on a word
                    bool isDouble = IsDoubleClick(pressStartWord);
                    RecordClick(pressStartWord);
                    if (onTap) onTap(pressStartWord, pos, isDouble);
                } else if (pressStartWord < 0 && !didScroll_) {
                    // Tap on empty space
                    if (onTapEmpty) onTapEmpty(pos);
                }
                pressState = Idle;
            } else if (GetTime() - pressStartTime > LONG_PRESS_TIME) {
                // Long press — let App decide what to do
                if (pressStartWord >= 0 && onLongPress)
                    onLongPress(pressStartWord, pos);
                pressState = LongPress;
            } else {
                float dx = pos.x - pressStartPos.x;
                float dy = pos.y - pressStartPos.y;
                float distSq = dx*dx + dy*dy;
                if (distSq > SLOP_THRESHOLD_SQ) {
                    if (pressStartWord >= 0) {
                        pressState = Dragging;
                        if (onDragStart) onDragStart(pressStartWord, pos);
                    } else {
                        pressState = Idle; // scroll started on empty space
                    }
                }
            }
            break;

        case Dragging:
            if (isPressed) {
                int wordId = hitTestFn ? hitTestFn(pos.x, pos.y) : -1;
                if (wordId >= 0 && onDragUpdate)
                    onDragUpdate(pressStartWord, wordId, pos);
            }
            if (justReleased) {
                int endWord = hitTestFn ? hitTestFn(pos.x, pos.y)
                                        : pressStartWord;
                if (onDragEnd) onDragEnd(pressStartWord, endWord, pos);
                pressState = Idle;
            }
            break;

        case LongPress:
            if (justReleased) pressState = Idle;
            break;
    }
}
```

Key unification decisions:
- **Drag detection**: Uses 2D (dx+dy) like desktop, not just dy like touch — desktop behavior is more correct for selection drag
- **Long-press**: Fires unconditionally on any word; App decides what to do (highlighted → show menu, not highlighted → start selection)
- **`didScroll_`**: Instead of `touchScrollEmitted_` + `touchActive` interaction, just one `bool didScroll_` set by `HandleTouchScroll` when slop exceeds threshold, checked on release
- **No `touchConsumedByRadial_`**: App handles all radial lifecycle via `onTap`/`onTapEmpty`. The FSM has no knowledge of the radial menu.

#### Change 2: Semantic Callbacks Replace Lifecycle Callbacks

Remove from `InputHandler`:
```
Poll(..., radialClickHandler, radialDismiss, radialShowHandler)  // remove all 3
```

Add to `InputHandler` as constructor params or setters:
```
onTap(wordId, pos, isDoubleTap)         // word pressed & released, minimal movement
onTapEmpty(pos)                         // empty space pressed & released
onDragStart(startWord, pos)             // started a drag on a word
onDragUpdate(startWord, currentWord, pos) // drag finger moved
onDragEnd(startWord, endWord, pos)      // drag released
onLongPress(wordId, pos)                // long-press detected
onScroll(delta, velocity)               // mouse wheel or touch scroll
onDismiss()                             // Escape key
```

Callers (currently only `App.cpp`) subscribe only to what they need.

#### Change 3: App.cpp Single `onTap` Handler

```cpp
auto onTap = [this](int wordId, Vector2 pos, bool isDouble) {
    if (uiManager_->IsRadialMenuActive()) {
        auto result = uiManager_->HandleRadialMenuClick(pos);
        if (result.consumed) {
            // Action (copy/delete/highlight) — done
            accumStartWord_ = -1;
            accumEndWord_ = -1;
            return;
        }
        // Missed buttons
        if (wordId >= 0 && accumStartWord_ >= 0) {
            // Expansion mode — extend selection
            ExpandSelection(wordId);
            return; // menu stays
        }
        if (wordId >= 0) {
            // Single word tap — show menu for this word
            ShowMenuForWord(wordId, pos);
            return;
        }
        // Empty space — dismiss
        uiManager_->HideRadialMenu();
        accumStartWord_ = -1;
        accumEndWord_ = -1;
        return;
    }

    // No menu active
    if (isDouble) {
        // Double-click — select verse, show menu, enter expansion mode
        auto* data = docManager_->GetCurrentChapterData();
        int vStart, vEnd;
        FindVerseRange(data->words, wordId, vStart, vEnd);
        accumStartWord_ = vStart;
        accumEndWord_ = vEnd;
        highlighter_->CommitSelection(vStart, vEnd);
        uiManager_->ShowRadialMenu(pos, vStart, vEnd);
    } else if (highlighter_->HighlightAtWord(wordId)) {
        // Single tap on highlighted word — show menu
        const auto* h = highlighter_->HighlightAtWord(wordId);
        accumStartWord_ = -1;
        accumEndWord_ = -1;
        highlighter_->CommitSelection(h->startWord, h->endWord);
        uiManager_->ShowRadialMenu(pos, h->startWord, h->endWord);
    } // else: single tap on unhighlighted word — nothing
};
```

No touch-down race. No flicker. Everything in one place.

#### Change 4: Flag State Reduction

| Current (15) | Future |
|---|---|
| `pressState` | **Keep** |
| `pressStartTime` | **Keep** |
| `pressStartPos` | **Keep** |
| `pressStartWord` | **Keep** |
| `selectStartWord` | **Keep** (for long-press drag) |
| `touchActive` | Replaced by per-frame `wasPressed` / `prevPressed_` |
| `touchLastY` | Scoped to scroll (local in HandleTouchScroll) |
| `touchLaunchVelocity_` | Scoped to scroll (local in HandleTouchScroll or scroll member) |
| `lastPinchDist` | Scoped to pinch (local in HandlePinch) |
| `touchScrollEmitted_` | Replaced by `didScroll_` (single bool, checked on release) |
| `lastTouchPos_` | Scoped to scroll/touch — no longer needed by tap FSM |
| `lastClickTime_` | **Keep** (double-tap) |
| `lastClickWord_` | **Keep** (double-tap) |
| `touchConsumedByRadial_` | **Remove** |
| `dialogActive_` | **Keep** but FSM resets to Idle when dialog opens |

**Net reduction: ~5 removed, ~3 scoped = ~12 total** (from 15).

---

## Part 4: Implementation Plan

### Step 0: Document Design Decisions
- Add code comment at top of `InputHandler::Poll()` documenting the execution order and intent of each subsystem
- Rationale: P12 (poll ordering) has been implicit; make it explicit

### Step 1: Unify the Two FSMs
- `InputHandler.cpp`: Merge `HandlePressFSM` + `HandleTouchPressFSM` → `RunUnifiedFSM()`
- Abstract `isPressed()`, `justPressed()`, `justReleased()`, `getPos()` as local lambdas chosen at top of `Poll()`
- Unify drag detection: use 2D (desktop behavior), not just dy
- Unify long-press: unconditional on any word; let App decide behavior
- Handle `Selecting` state (touch long-press drag): fold into unified `LongPress` state, emit `onLongPress` callback, App sets up selection tracking
- Result: single FSM, zero duplication, all 14 differences resolved

### Step 2: Replace Dirty Flags with Release-Time Checks
- Remove `touchConsumedByRadial_` — App handles all radial lifecycle via `onTap`
- Remove `touchActive` — replace with per-frame `bool prevPressed_` (just `Poll()` local)
- Remove `touchScrollEmitted_` — replace with `bool didScroll_` set by `HandleTouchScroll` when slop exceeded
- `HandleTouchScroll` resets `didScroll_` on touch-start edge (or it's naturally per-touch)
- Ensure `slopAccumulator` always reset on any touch-start edge (fix the radial-dismiss path that skips the reset)

### Step 3: Replace Three Lifecycle Callbacks with Semantic Callbacks
- `InputHandler.h`: Remove `RadialMenuClickCallback`, `RadialMenuShowCallback`, `ContextDismissHandler`
- Add `onTap`, `onTapEmpty`, `onDragStart`, `onDragUpdate`, `onDragEnd`, `onLongPress`, `onScroll`, `onDismiss` as `std::function` members
- Remove them from `Poll()` signature — Poll takes `deltaTime` and `navStack*` only
- Wire semantic callbacks at FSM transition points inside `RunUnifiedFSM()`
- Keep `HandleTouchScroll()` separate (it emits `ScrollEvent` on EventBus directly)
- Keep `HandlePinch()` separate

Design decision — **callbacks vs EventBus**:
- Option A (callbacks): Simple, direct. App subscribes at InputHandler construction. Works fine for single-consumer.
- Option B (EventBus events): Extensible. Future modules (analytics, test recorder) could observe taps without modifying InputHandler. But adds event struct definitions and bus wiring.
- **Recommendation**: Start with Option A (callbacks) for simplicity. The EventBus can be added later by wrapping callbacks → event emissions in App. Document this in the code.

### Step 4: Consolidate App.cpp Radial Lifecycle into Single `onTap` Handler
- `App.cpp`: Remove `radialClickHandler`, `radialShowHandler`, `radialDismiss` lambdas
- Replace with single `onTap` + `onTapEmpty` handler (§3.2 Change 3)
- Move all `accumStartWord_` / `accumEndWord_` logic into this single handler
- Wire `InputHandler` with new callbacks at construction or before Poll loop
- Double-check: all call sites → 9 previous call sites now handled by 1-2 callbacks

### Step 5: Fix `dialogActive_` Freeze
- When `dialogActive_` transitions false → true, reset `pressState = Idle`
- Simple: before the early return `if (dialogActive_)`, add `pressState = Idle;`
- Also reset `prevPressed_ = false;` to avoid stale "released" detection when dialog closes

### Step 6: Fix Coordinate Inconsistency
- Inside `RunUnifiedFSM()`, the `getPos()` abstraction naturally uses `GetTouchPosition(0)` on touch and `GetMousePosition()` on desktop
- Remove the standalone `GetMousePosition()` call at the old `radialClickHandler` site (it's gone in Step 3)
- `HandleTouchScroll` and `HandlePinch` already use `GetTouchPosition` — no change needed

### Step 7: Remove Dead Events (Opportunistic Cleanup)
- `event/Events.h`: Remove `RightClickEvent` and `ScrollStopEvent` structs
- Search for any remaining references in the codebase (unlikely to exist since they're never emitted)
- All tests pass (these events aren't used in tests either)

### Step 8: Create `TapDetector` Helper for Screens (Opportunistic)
- `ui/TapDetector.h`: Small utility class encapsulating the press→drag→release pattern
```cpp
class TapDetector {
    Vector2 pressStartPos_{};
    bool hasPendingPress_ = false;
    float thresholdSq_;
public:
    explicit TapDetector(float slopDp = 10.0f);
    bool HandlePress(Vector2 pos);        // call on IsMouseButtonPressed
    bool HandleRelease(Vector2 pos);      // returns true if it was a tap
    bool IsDragging(Vector2 currentPos);  // returns true if exceeded slop
    void Reset();
};
```
- Refactor `CenterMenu`, `BookListScreen`, `HighlightBrowserScreen`, `SettingsScreen`, `CreditsOverlay`, `ChapterGridScreen` to use it
- Removes ~60 lines of duplication and 12 member fields

---

## Part 5: Verification

### Build Targets
- [ ] Desktop build: 0 warnings
- [ ] Android APK (arm64-v8a): 0 warnings

### Tests
- [ ] 70/72 tests pass (same locale failures)
- [ ] The two locale failures unchanged

### Behavioral Verification
| Scenario | Desktop (mouse) | Touch (Android) |
|----------|:---:|:---:|
| Tap highlighted word → radial menu | ✓ | ✓ |
| Tap unhighlighted word → nothing | ✓ | ✓ |
| Double-click on unhighlighted word → expansion mode | ✓ | ✓ |
| Single-tap another verse while menu visible → expansion | ✓ | ✓ |
| Drag selection → radial menu on release | ✓ | N/A |
| Long-press on word → selection mode | ✓ | ✓ |
| Long-press on highlighted word → menu | ✓ | N/A (tap works) |
| Right-click on word → menu | ✓ | N/A |
| Scroll while menu visible → menu stays | ✓ | ✓ |
| Tap outside menu → dismiss | ✓ | ✓ |
| Escape key → dismiss | ✓ | ✓ |
| Dialog opens mid-drag → no freeze | ✓ | ✓ |

---

## Part 6: Risks & Mitigations

| Risk | Mitigation |
|------|------------|
| Unified FSM changes timing behavior of long-press | Same timer (LONG_PRESS_TIME = 300ms); only the dispatch mechanism changes |
| `onTap` fires on release instead of press — feels slower | Already how FSM works internally (tap = press+release without scroll). No net latency. |
| Large refactor touches 3 core files + 7 screen files + 1 event file | All changes are mechanical (unify → restructure → remove). No logic changes to radial menu, highlight system, or document rendering. |
| Screens' pressStartPos_ → TapDetector migration is boring + error-prone | Do it in a separate commit. Can verify screen by screen. |
| EventBus events vs callbacks — wrong choice leads to rework | Start with callbacks (simple, sufficient). If events are needed later, wrap `onTap` → emit `TapEvent` in App — no InputHandler changes. |
| Scroll inertia coupling with FSM | `HandleTouchScroll` remains independent. `didScroll_` is the only coupling point (one bool). |
| Platform-specific behavior (desktop long-press vs touch long-press) | Unified FSM fires `onLongPress` for any word. App decides: on desktop with highlighted word → show menu; on touch → start drag selection. The behavior is driven by the callback, not by the FSM. |

---

## Summary: What Changes in Each File

| File | Lines | Change |
|------|:-----:|--------|
| `InputHandler.h` | ~30 | Remove 3 lifecycle callbacks, add ~7 semantic callbacks; remove `touchConsumedByRadial_`; add `didScroll_`, `prevPressed_`; remove `touchActive`, `touchScrollEmitted_` |
| `InputHandler.cpp` | ~270 | Replace dual FSMs with `RunUnifiedFSM()`; semantic event emission; dialog freeze fix; coordinate abstraction |
| `App.cpp` | ~100 | Replace 3 lambdas with single `onTap`/`onTapEmpty`; accumulator consolidated |
| `event/Events.h` | ~5 | Remove `RightClickEvent`, `ScrollStopEvent` |
| `ui/TapDetector.h` | ~40 | New file — reusable press→drag→release utility |
| 6 screen `.h` files | ~12 | Remove `pressStartPos_`, `hasPendingPress_` fields |
| 6 screen `.cpp` files | ~60 | Use `TapDetector` instead of inline pattern |
