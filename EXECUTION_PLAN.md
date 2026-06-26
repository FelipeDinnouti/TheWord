# Re-architecture Migration — Execution Plan

> Status: Active | Created: 2026-06-26
>
> Implements the design in `the-word-docs/07-ai-collaboration/Full Re-architecture Proposal.md`.

---

## Execution Order

```
Phase 1 (Event Infrastructure)
    ↓
Phase 2 (Input-Output Decoupling) — biggest change
    ↓
Phase 3 (App Class Extraction)
    ↓
Phase 4 (UIManager Refinement)
    ↓
Phase 5 (Namespace Migration) — mechanical, touches every file
    ↓
Phase 6 (Platform Extensions) — optional, independent
```

---

## Phase 1 — Event Infrastructure

**Goal**: Create the `event/` module with EventBus and event structs. No behavioral changes.

**Files created**:
- `src/event/EventBus.h` — type-safe, header-only event bus (~40 lines)
- `src/event/Events.h` — all event structs (ScrollEvent, ResizeEvent, FontSizeEvent, etc.)

**Files modified**:
- `CMakeLists.txt` — add `src/event/` source directory with header files

**Verification**: `cmake --build build && ./build/theword_test` — 64/64 pass, app runs identically.

---

## Phase 2 — Input-Output Decoupling (one-shot)

**Goal**: Inject EventBus into all modules. InputHandler emits events instead of calling methods directly. Consumers subscribe to their events. No dual-path — single atomic change.

**Why one-shot instead of dual-path**: Dual-path risks double-execution (e.g., `ScrollBy` called both via direct call and via event). One-shot is cleaner to verify — either events work or they don't. The risk is mitigated by:
- EventBus is ~40 lines, trivially testable in isolation
- 64 tests exercise the full pipeline
- Each event subscription is a simple `On<T>(callback)` in the consumer's constructor

### What changes in each file

**InputHandler** (`src/input/InputHandler.h/.cpp`):
- Constructor: adds `EventBus& eventBus`, removes `DocumentManager&`, `Highlighter&`, `LayoutEngine&`, `UIManager&`
- `HandleInput` → `Poll(float deltaTime)`: keeps all FSM logic (mouse state machine, touch gestures, keyboard shortcuts) but emits events instead of calling methods:
  - `docManager.ScrollBy(delta)` → `eventBus.Emit(ScrollEvent{delta})`
  - `highlighter.StartSelection(...)` → `eventBus.Emit(SelectionEvent{...})`
  - `uiManager.DismissActiveDialog()` → `eventBus.Emit(KeyEvent{key::ESCAPE})`
  - `layoutEngine.SetMaxWidth(w)` → `eventBus.Emit(ResizeEvent{w, h, ...})`
- Dialog active check: InputHandler maintains a local `bool dialogActive_` flag, updated via `DialogEvent` subscription
- Removes all module member references

**DocumentManager** (`src/document/DocumentManager.h/.cpp`):
- Constructor: adds `EventBus& eventBus`
- Subscribes to: `ScrollEvent`, `ResizeEvent`, `FontSizeEvent`, `SourceSwitchEvent`

**Highlighter** (`src/highlight/Highlighter.h/.cpp`):
- Constructor: adds `EventBus& eventBus`
- Subscribes to: `SelectionEvent`

**LayoutEngine** (`src/text/LayoutEngine.h/.cpp`):
- Constructor: adds `EventBus& eventBus`
- Subscribes to: `ResizeEvent`, `FontSizeEvent`

**Renderer** (`src/renderer/Renderer.h/.cpp`):
- Constructor: adds `EventBus& eventBus`
- Subscribes to: `FontSizeEvent`

**UIManager** (`src/renderer/UIManager.h/.cpp`):
- Constructor: adds `EventBus& eventBus`
- Subscribes to: `KeyEvent`, `DialogEvent`
- Emits: `FontSizeEvent`, `SourceSwitchEvent` (but still also calls the old methods — dual-path here is intentional to keep Phase 2 atomic; Phase 4 removes the old paths)

**main.cpp**:
- Creates `EventBus`, passes it to all constructors

**Verification**: `cmake --build build && ./build/theword_test` — 64/64 pass, app runs identically (scroll, click, resize, G/S/A keyboard shortcuts all work).

---

## Phase 3 — App Class Extraction

**Goal**: Create `src/app/App.h/.cpp` to encapsulate all top-level state and wiring. `main.cpp` becomes 5 lines.

**Files created**:
- `src/app/App.h` — `App` class declaration
- `src/app/App.cpp` — `Init()`, `WireEvents()`, `Run()`, `Update()`, `Draw()`

**Files modified**:
- `src/main.cpp` — reduce to `App app; app.Init("TheWord"); app.Run();`
- `CMakeLists.txt` — add `src/app/` source directory

**Move from main.cpp to App::Init()**:
- All object construction (PersistenceManager, USFMParser, BibleClient, etc.)
- All font loading
- All initial chapter loading (`LoadInitialChapter("GEN.1")`)
- All event wiring into `WireEvents()`:
  - Cross-cutting `FontSizeEvent` orchestration (LayoutEngine + DocumentManager + Renderer)
  - Cross-cutting `SourceSwitchEvent` orchestration (CompositeProvider + DocumentManager + PersistenceManager)
  - InputHandler dialog-active flag tracking

**Verification**: `cmake --build build && ./build/theword_test` — 64/64 pass, app runs identically.

---

## Phase 4 — UIManager Refinement

**Goal**: Remove `DocumentManager&`, `LayoutEngine&`, `Renderer&` from UIManager. These cross-cutting responsibilities move to `App::WireEvents()`.

**Files modified**:
- `src/renderer/UIManager.h` — remove `DocumentManager&`, `LayoutEngine&`, `Renderer&` member refs
- `src/renderer/UIManager.cpp`:
  - `ChangeFontSize()` — emit `FontSizeEvent` only, remove 4 direct method calls
  - `ApplyFontSize()` — remove (handled by event subscribers)
  - `SwitchSource()` — emit `SourceSwitchEvent` only, remove orchestration
- `src/app/App.cpp` — `WireEvents()` adds `FontSizeEvent` and `SourceSwitchEvent` subscribers that do the cross-cutting work

**What moves to App::WireEvents()**:
```cpp
// Font size change — one subscriber replaces 4 direct calls
eventBus->On<FontSizeEvent>([&](const auto& e) {
    layoutEngine->SetFontSize(e.newSize);
    layoutEngine->InvalidateCache();
    docManager->InvalidateLayouts();
    renderer->SetFontSize(e.newSize);
    uiManager->OnFontSizeApplied(e.newSize);  // UI-only: refresh label
});

// Source switch
eventBus->On<SourceSwitchEvent>([&](const auto& e) {
    compositeProvider->SetPrimary(e.online ? 0 : 1);
    // Reload current chapter from new source
    auto currentRef = docManager->GetCurrentChapterRef();
    docManager->ReloadChapter(currentRef.bookId, currentRef.chapter);
    persistence->SavePreference("active_version", e.online ? "1" : "0");
});
```

**Verification**: `cmake --build build && ./build/theword_test` — 64/64 pass, font size buttons work, source toggle works, app runs identically.

---

## Phase 5 — Namespace Migration

**Goal**: Wrap every class and entity in `theword::<module>::` namespace.

**Scope**: Every `.h` and `.cpp` file under `src/` (excluding `src/main/java/`).

**Pattern per header**:
```cpp
#ifndef PLATFORM_H
#define PLATFORM_H

#include <string>
// ... other includes

namespace theword::core {
    class Platform {
        // ... same as before
    };
}
#endif
```

**Pattern per source**:
```cpp
#include "core/Platform.h"
#include <cstdio>

namespace theword::core {
    // Implementation
}
```

**using declarations**:
- Headers: fully qualified names only
- `.cpp` files: single `using namespace theword::renderer;` at top for own module; qualified names for cross-module refs

**Files excluded**:
- `src/main.cpp` — uses fully qualified names to construct objects
- `src/main/java/` — Java files, no C++ namespaces
- `tests/` — tests use fully qualified names
- `src/event/Events.h` — structs already in `theword::event` namespace

**Verification**: `cmake --build build && ./build/theword_test` — 64/64 pass, app runs.

---

## Phase 6 — Platform Extensions (optional)

**Goal**: Add `platform::OpenURL()`, abstracted log sink, clipboard. Independent of other phases.

**Files modified**:
- `src/core/Platform.h` — declare `OpenURL`, `WriteLog`, `GetClipboard`/`SetClipboard`
- `src/core/Platform.cpp` — implement with `#ifdef` per platform
- `src/core/Logger.cpp` — remove 5 `#ifdef __ANDROID__` blocks, call `platform::WriteLog()` instead

**Verification**: `cmake --build build && ./build/theword_test` — 64/64 pass.

---

## Summary

| Phase | Files created | Files modified | Lines changed (est.) | Risk |
|-------|---------------|----------------|---------------------|------|
| 1 | 2 | 1 | +50 | None |
| 2 | 0 | 12+ | ~200 | Medium |
| 3 | 2 | 2 | +180 / −180 | Low |
| 4 | 0 | 3 | ~50 | Low |
| 5 | 0 | ~35 | ~500 (mechanical) | Low |
| 6 | 0 | 2 | ~40 | Low |

**Total**: ~1000 lines changed across ~40 files. Each phase independently verifiable.
