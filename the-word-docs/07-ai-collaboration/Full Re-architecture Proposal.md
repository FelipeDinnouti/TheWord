# Full Re-architecture Proposal

> Status: Draft Proposal | Last Updated: 2026-06-26
>
> This document describes the **ideal future state** of the codebase. It is a
> planning artifact, not a current spec. The migration is designed to be
> incremental — each step leaves the app working and tests passing.

---

## Table of Contents

1. [Current State Assessment](#1-current-state-assessment)
2. [Namespace Strategy](#2-namespace-strategy)
3. [Module Boundaries & Ownership](#3-module-boundaries--ownership)
4. [Platform Abstraction Layer — Extensions](#4-platform-abstraction-layer--extensions)
5. [Event System Design](#5-event-system-design)
6. [Ideal Component Splitting](#6-ideal-component-splitting)
7. [Migration Path](#7-migration-path)

---

## 1. Current State Assessment

### What works well

| Aspect | Current state | Keep |
|--------|---------------|------|
| Module layers | `core → data → text → document → renderer` — mostly acyclic | Yes |
| `Renderer` | Pure sink — no doc/input refs, just draws what it's given | Yes |
| `Platform.h` | `Init()` returns `Info{dpiScale, dbPath, assets}` — consolidates 15 `#ifdef` blocks | Yes, extend |
| `PersistenceInterface` | Clean abstract base in `highlight/`, `PersistenceManager` in `persistence/` implements it | Yes |
| `IAssetProvider` | Abstract file-reader, 2 platform impls | Yes |
| Data types | `Word`, `Span`, `Segment`, `ChapterData` defined once in `data/ChapterProvider.h` | Yes |

### What needs to change

| Issue | Location | Severity |
|-------|----------|----------|
| No namespaces | Every class in global scope | Medium |
| `UIManager` depends on `DocumentManager` | `renderer/UIManager.cpp` includes `document/DocumentManager.h` — reverse dependency | High |
| `UIManager` is a god object | 7 injected refs + 4 owned sub-dialogs + font size + source switching | High |
| `InputHandler` directly mutates 4 modules | Calls methods on `DocumentManager`, `Highlighter`, `LayoutEngine`, `UIManager` | High |
| No event system | Resize, scroll, font changes flow through direct method calls | Medium |
| `main.cpp` does all wiring | ~200 lines of manual DI, no `App` class | Low |
| Platform features are minimal | No `OpenURL`, no abstracted logger, no clipboard | Low |

---

## 2. Namespace Strategy

### Principle

Every class and namespace-level entity lives under `theword::<module>::`. This:
- Eliminates global namespace pollution
- Makes module ownership explicit in code
- Follows C++17 convention for non-trivial projects
- Enables forward declarations by known path (`namespace theword::core { class Platform; }`)

### Mapping

| Module | Namespace | Contents |
|--------|-----------|----------|
| `core/` | `theword::core` | `Platform`, `Config`, `Theme`, `Logger`, `EnvLoader`, `IAssetProvider`, `FileAssetProvider`, `AndroidAssetProvider`, `IHttpClient`, `CurlHttpClient`, `EmscriptenClient`, `FontHelper`, `GlobalId`, `BibleBooks`, `BookInfo` |
| `data/` | `theword::data` | `ChapterProvider`, `USFMParser`, `BibleClient`, `CompositeProvider`, `StubChapterProvider`, `DataUtils` |
| `text/` | `theword::text` | `LayoutEngine` |
| `document/` | `theword::document` | `DocumentManager`, `LoadedChapter` |
| `highlight/` | `theword::highlight` | `Highlighter`, `PersistenceInterface`, `InMemoryStorage`, `SimpleColor`, `Highlight`, `HighlightType` |
| `persistence/` | `theword::persistence` | `PersistenceManager` |
| `renderer/` | `theword::renderer` | `Renderer`, `UIManager`, `ContextMenu`, `SettingsPanel`, `GoToDialog`, `AboutOverlay`, `HighlightRect` |
| `input/` | `theword::input` | `InputHandler` |
| `app/` | `theword::app` | `App` (new class proposed in §3) |
| `event/` | `theword::event` | `EventBus`, all event structs (new module proposed in §5) |

### Flat namespace exceptions

The namespaces `config`, `key`, and `theme` (defined in `core/Config.h` and `core/Theme.h`)
remain flat under `theword::core`:
- `theword::core::config::WINDOW_WIDTH`
- `theword::core::key::ESCAPE`
- `theword::core::theme::COLOR_BACKGROUND`

These are constants-only namespaces; nesting them under an additional layer would add
verbosity with no benefit.

### Using declarations

- **Headers**: Always use fully-qualified names (`theword::core::Platform`). Never
  `using namespace` in a header — it leaks into every translation unit that includes it.
- **`.cpp` files**: A single `using namespace theword::renderer;` at the top of each
  implementation file after includes is acceptable (local scope only). For cross-module
  references, prefer qualified names or specific `using` declarations.

### Forward declarations

Since namespaces are now nested, forward declarations use:

```cpp
// Forward declare a class from another module
namespace theword::document { class DocumentManager; }
namespace theword::input    { class InputHandler; }
```

Class methods that take cross-module parameters forward-declare at the top of their
header, following existing patterns.

---

## 3. Module Boundaries & Ownership

### Current dependency graph

```
main.cpp  ──→  core  ──→  data  ──→  text  ──→  document  ──→  renderer
   │                    ↕                        ↕              ║  ↑
   └──→ input ──→  highlight ∙ persistence    UIManager ────────╝  │
                                                     ║              │
                                                     ╚══→ document  │
                                                          (violation) │
                                          Renderer ───────────────────┘
                                                       (pure sink, OK)
```

**Violation**: `UIManager` (in `renderer/`) includes `DocumentManager.h` (in `document/`).
This creates a cycle: `document → text → data` and `renderer → document`.

### Proposed dependency graph

```
                    ┌──────────────────┐
                    │      App         │
                    │  (orchestrator)  │
                    └────────┬─────────┘
                             │
         ┌───────────────────┼───────────────────┐
         ▼                   ▼                   ▼
    ┌─────────┐    ┌─────────────────┐    ┌──────────┐
    │  event  │    │   InputHandler  │    │ UIManager│
    │  bus    │    │  (emits events) │    │(emits &  │
    └─────────┘    └────────┬────────┘    │ subscribes)
         ▲                   │             └─────┬────┘
         │                   ▼                   │
         │           ┌───────────────┐           │
         ├───────────│ DocumentMgr   │◄──────────┤
         │           │ Highlighter   │           │
         │           │ LayoutEngine  │           │
         │           │ Renderer      │           │
         │           └───────────────┘           │
         │                                       ▼
         └───────────────────────────────┐ ┌────────────┐
                                         │ │ Sub-dialogs│
                                         │ │ (ContextM, │
                                         │ │  SettingsP,│
                                         │ │  GoTo,     │
                                         │ │  About)    │
                                         │ └────────────┘
                                         ▼
                              ┌─────────────────────┐
                              │ core → data → text │
                              │ (no change)         │
                              └─────────────────────┘
```

**Changes**:
- `renderer/` no longer depends on `document/`
- `input/` no longer depends on `document/`, `highlight/`, `renderer/`
- Cross-cutting communication flows through the `event` bus
- `App` (new module in `src/app/`) owns all top-level state and wiring

### The App class

New file: `src/app/App.h` + `src/app/App.cpp`

```cpp
namespace theword::app {

class App {
public:
    App();
    bool Init(const char* title);          // replaces main() wiring
    bool ShouldQuit() const;               // replaces platform::ShouldQuit
    void Update(float deltaTime);          // processes event queue + smooth scroll
    void Draw();                           // BeginDrawing / EndDrawing wrapper
    void Run();                            // frame loop: Update + Draw

private:
    // Owned subsystems — constructed in Init(), destroyed in ~App()
    std::unique_ptr<core::Platform>        platform_;       // wraps Platform.h
    std::unique_ptr<event::EventBus>       eventBus_;
    std::unique_ptr<data::USFMParser>      usfmParser_;
    std::unique_ptr<data::BibleClient>     bibleClient_;
    std::unique_ptr<data::CompositeProvider> compositeProv_;
    std::unique_ptr<persistence::PersistenceManager> persistence_;
    std::unique_ptr<hierarchy::Highlighter>    highlighter_;
    std::unique_ptr<text::LayoutEngine>    layoutEngine_;
    std::unique_ptr<renderer::Renderer>    renderer_;
    std::unique_ptr<document::DocumentManager> docManager_;
    std::unique_ptr<renderer::UIManager>   uiManager_;
    std::unique_ptr<input::InputHandler>   inputHandler_;

    // Event wiring (subscriptions)
    void WireEvents();
};

} // namespace theword::app
```

`main.cpp` becomes:

```cpp
#include "app/App.h"
int main() {
    theword::app::App app;
    if (!app.Init("TheWord")) return 1;
    app.Run();
    return 0;
}
```

### Clear ownership per module

| Module | Owns | Does NOT own |
|--------|------|--------------|
| `data/` | Chapter data, HTTP, USFM parsing | UI state, input state, highlight state |
| `text/` | Layout cache, word wrapping | Document state, scroll state |
| `document/` | Chapters, scroll position, viewport | Font size, source selection, highlight data |
| `highlight/` | Selection state, word→color mapping | UI rendering, persistence schema |
| `persistence/` | SQLite connection, schema, queries | Business logic (who highlights what) |
| `input/` | Event emission only | No module knowledge |
| `renderer/` | Draw calls, font refs, scrollbar | Document data, input state |
| `ui/` (UIManager) | Sub-dialog lifecycle, UI local state | Document, layout, renderer refs |
| `app/` | Wiring, event bus, frame loop | Everything below |

---

## 4. Platform Abstraction Layer — Extensions

### Current state (keep)

`Platform.h` and `Platform.cpp` already consolidate:
- `platform::Init()` → `Info{dpiScale, dbPath, std::unique_ptr<IAssetProvider>}`
- `platform::CreateHttpClient()` → `IHttpClient*` or `nullptr`
- `platform::ShouldQuit()` → Android window check (no-op elsewhere)

### Proposed additions

#### 4.1 `platform::OpenURL(const char*)`

Used when the user clicks an external Bible reference link. Platform dispatch:
- **Linux**: `xdg-open <url>`
- **macOS**: `open <url>`
- **Windows**: `ShellExecute`
- **Android**: `JNI → Intent(ACTION_VIEW)`
- **WASM**: `window.open(url, "_blank")`

#### 4.2 `platform::GetClipboard()` / `platform::SetClipboard()`

Raylib provides `GetClipboardText()` / `SetClipboardText()`, but on Android
the clipboard lives in the Java layer. Platform abstraction provides:
- **Desktop/WASM**: passes through to raylib
- **Android**: JNI call to `ClipboardManager`

#### 4.3 Abstracted log sink

Currently `Logger.cpp` has 5 `#ifdef __ANDROID__` blocks for `__android_log_print()`
vs `fprintf(stderr)`. Move this behind a platform log sink:

```cpp
namespace theword::core::platform {
    enum LogLevel { DEBUG, INFO, WARN, ERROR };
    void WriteLog(LogLevel level, const char* message);
}
```

`Logger` calls `platform::WriteLog` unconditionally. Each platform provides its
implementation — the `#ifdef` stays in a single platform dispatch point.
This eliminates the remaining `#ifdef` blocks from `Logger.cpp`.

#### 4.4 `platform::AppDataDir()`

`Info.dbPath` already serves this purpose (the directory containing the SQLite DB).
Formalize it as `Info::appDataDir` (the directory) and `Info::dbPath` (the file).

### Platform module location

All platform code stays in `src/core/Platform.h/.cpp`. The extensions above are
implemented as additional free functions in `theword::core::platform::` namespace.

---

## 5. Event System Design

### EventBus class

A lightweight, header-only, type-safe event bus. ~40 lines of real code.

```cpp
// src/event/EventBus.h
#ifndef EVENTBUS_H
#define EVENTBUS_H

#include <any>
#include <functional>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace theword::event {

class EventBus {
public:
    template<typename T>
    using Slot = std::function<void(const T&)>;

    template<typename T>
    void On(Slot<T> slot) {
        auto& entries = slots_[std::type_index(typeid(T))];
        entries.push_back(std::make_shared<std::function<void(const std::any&)>>(
            [s = std::move(slot)](const std::any& e) {
                s(std::any_cast<const T&>(e));
            }
        ));
    }

    template<typename T>
    void Emit(const T& event) {
        auto it = slots_.find(std::type_index(typeid(T)));
        if (it != slots_.end()) {
            for (auto& slot : it->second) {
                (*slot)(std::any(event));
            }
        }
    }

private:
    std::unordered_map<
        std::type_index,
        std::vector<std::shared_ptr<std::function<void(const std::any&)>>>
    > slots_;
};

} // namespace theword::event
#endif
```

**Design rationale**:
- **Type-safe**: `On<T>(callback)` and `Emit<T>(event)` match on `std::type_index(typeid(T))`.
  A `ScrollEvent` subscriber never receives a `ResizeEvent`.
- **No dependencies**: Uses only `std::any`, `std::type_index`, `std::function`. No external
  libraries. Tested with `static_assert` that the erased type matches.
- **Shared ownership**: The `std::shared_ptr` wrapping each slot allows safe removal
  or replacement in the future (add `Disconnect()` when needed).
- **Header-only**: Zero compilation overhead. Include and use.

### Event structs

Each event is a plain struct (POD or with simple members). No methods, no inheritance.

```cpp
// src/event/Events.h
namespace theword::event {

struct ScrollEvent       { float delta; };
struct ResizeEvent       { int width; int height; int prevScrollY; };

struct SelectionEvent {
    enum class Action { Start, Update, End, Cancel } action;
    int startWord;
    int endWord;
    int chapterGlobalId;        // for filtering by visible chapter
};

struct FontSizeEvent      { float newSize; };
struct SourceSwitchEvent  { bool online; };

struct DialogEvent {
    enum class Type { GoTo, Settings, About, ContextMenu } type;
    enum class Action { Show, Hide, Toggle } action;
};

struct KeyEvent           { int key; int mods; };
struct NavigateEvent      { const char* bookId; int chapter; };

} // namespace theword::event
```

### Event flow (per frame)

```
App::Run()
  │
  ├─ InputHandler::Poll()          ── emits: ScrollEvent, SelectionEvent,
  │                                        KeyEvent, DialogEvent, ResizeEvent
  │
  ├─ EventBus::Process()           ── dispatches to subscribers:
  │    ├── DocumentManager::OnScroll  ← ScrollEvent
  │    ├── Highlighter::OnSelection   ← SelectionEvent
  │    ├── UIManager::OnKey           ← KeyEvent / DialogEvent
  │    ├── LayoutEngine::OnResize     ← ResizeEvent
  │    └── Renderer::OnFontSize       ← FontSizeEvent
  │
  ├─ DocumentManager::Update(dt)  ── smooth scroll interpolation
  │
  └─ App::Draw()                  ── BeginDrawing → Renderer → UIManager → EndDrawing
```

### Subscription pattern

Event handlers follow a naming convention:

```cpp
// In DocumentManager constructor:
eventBus->On<event::ScrollEvent>([this](const auto& e) { OnScroll(e); });
eventBus->On<event::ResizeEvent>([this](const auto& e) { OnResize(e); });

// Handler methods — named "On<EventName>":
void DocumentManager::OnScroll(const event::ScrollEvent& e);
void DocumentManager::OnResize(const event::ResizeEvent& e);
```

---

## 6. Ideal Component Splitting

### 6.1 InputHandler → pure event emitter

| Current | Future |
|---------|--------|
| Holds references to `DocumentManager`, `Highlighter`, `LayoutEngine`, `UIManager` | Holds zero module references |
| Calls `docManager.ScrollBy()`, `highlighter.StartSelection()`, `uiManager.ShowContextMenu()` | Emits `ScrollEvent`, `SelectionEvent`, `KeyEvent`, `DialogEvent` |
| Handles `ResizeEvent` internally | Emits `ResizeEvent` to EventBus |
| Contains touch/pinch FSM inline | Same FSM, but emits events instead of calling methods |

**Signature**:
```cpp
// Current
void HandleInput(float deltaTime);

// Future
void Poll(float deltaTime);       // called each frame, emits events to EventBus
```

`InputHandler::Poll()` contains all the same FSM logic (mouse state machine,
touch gestures, keyboard shortcuts). The only difference is that instead of
calling methods on other modules, it calls `eventBus.Emit(ScrollEvent{delta})`.

### 6.2 UIManager → pure UI coordinator

| Current | Future |
|---------|--------|
| Holds `DocumentManager&`, `LayoutEngine&`, `Renderer&` | Holds only sub-dialog references + local state |
| `ChangeFontSize()` calls `layoutEngine.SetFontSize()`, `layoutEngine.InvalidateCache()`, `docManager.InvalidateLayouts()`, `docManager.ScrollTo()`, `renderer.SetFontSize()` | `ChangeFontSize()` emits `FontSizeEvent` — event bus dispatches to all subscribers |
| `SwitchSource()` calls `compositeProv->SetPrimary()`, `docManager.ReloadChapter()`, `persistence.SavePreference()` | `SwitchSource()` emits `SourceSwitchEvent` — App or mediator handles the orchestration |
| `Draw()` calls sub-dialogs' Draw methods | Same — this is fine |

The key insight: UIManager should **request** actions (font size change, source switch)
and let the event system distribute those requests to the modules that need to react.
UIManager does not need to know who reacts, only what it wants.

### 6.3 DocumentManager → event subscriber

| Current | Future |
|---------|--------|
| Methods called directly by InputHandler | Subscribes to `ScrollEvent`, `ResizeEvent` |
| `InvalidateLayouts()` called by UIManager after font change | Subscribes to `FontSizeEvent` |
| `ReloadChapter()` called by UIManager after source switch | Subscribes to `SourceSwitchEvent` (or receives navigation command via `NavigateEvent`) |

DocumentManager's public API shrinks:

```cpp
// Current: callers must know what to call
void ScrollBy(float delta);
void ScrollTo(float y);
void InvalidateLayouts();
std::optional<ChapterData> ReloadChapter(const std::string& bookId, int chapter);

// Future: internal event handlers
// (subscribes to events in constructor, no direct callers except App initial load)
```

DocumentManager remains the sole owner of scroll state and chapter data. The change
is purely in how it receives commands.

### 6.4 Renderer → event subscriber

| Current | Future |
|---------|--------|
| `SetFontSize(float)` called by UIManager | Subscribes to `FontSizeEvent` |

Renderer keeps its pure-sink nature. One event subscription replaces the
direct `renderer.SetFontSize()` call.

### 6.5 App → orchestrator

`App` is the only class that knows about all modules and the event bus.
Its responsibilities:

1. **Construction**: Create all subsystems in `Init()`
2. **Wiring**: Call `eventBus.On<T>(...)` to connect emitters to subscribers
3. **Frame loop**: `PollEvents()` → `ProcessEvents()` → `Update()` → `Draw()`
4. **Initial load**: `DocumentManager.LoadInitialChapter("GEN.1")`
5. **Lifecycle**: Handle `FontSizeEvent` / `SourceSwitchEvent` orchestration that
   requires cross-module coordination (if not cleanly handled by individual subscriptions)

### 6.6 Summary of component changes

| Component | Lines (now) | Lines (future) | Dependency count (now) | Dependency count (future) |
|-----------|-------------|----------------|----------------------|--------------------------|
| `InputHandler` | ~220 | ~200 | 4 module refs | 0 module refs (+ EventBus) |
| `UIManager` | ~180 | ~100 | 7 refs + 4 owned | 4 owned (+ EventBus) |
| `DocumentManager` | ~150 | ~160 | 2 refs | 2 refs (+ EventBus subscription) |
| `Renderer` | ~90 | ~95 | 0 module refs | 0 module refs (+ EventBus subscription) |
| `App` (new) | — | ~150 | — | owns all subsystems |
| `event/EventBus` (new) | — | ~40 | — | none (header-only) |
| `main.cpp` | ~200 | ~5 | ~15 includes | 1 include |

---

## 7. Migration Path

Each step is independently verifiable: build succeeds, tests pass, app runs.

### Step 1: Create EventBus + event structs

- Add `src/event/EventBus.h` and `src/event/Events.h`
- Add `event/` to CMakeLists source list
- No code changes to existing files
- **Verify**: build succeeds, 64/64 tests pass

### Step 2: Add event emission to InputHandler (dual-path)

- Inject `EventBus&` into `InputHandler` constructor
- Add event emissions alongside existing direct calls
- `Poll()` emits `ScrollEvent`, `SelectionEvent`, `KeyEvent`, etc. in addition to
  calling existing methods
- **Verify**: behavior unchanged (old path still active), build + tests pass

### Step 3: Add event subscriptions to consumers (dual-path)

- Inject `EventBus&` into `DocumentManager`, `Highlighter`, `UIManager`, `LayoutEngine`,
  `Renderer` constructors
- Subscribe to relevant events in each constructor
- Guard existing direct-call handlers: when the event also triggers, deduplicate
  (or let both paths run idempotently — e.g., `ScrollBy(0)` is harmless)
- **Verify**: behavior unchanged, build + tests pass

### Step 4: Remove old direct call paths from InputHandler

- Delete direct calls from `InputHandler::Poll()` — now only emits events
- Remove InputHandler's dependencies on `DocumentManager`, `Highlighter`, `LayoutEngine`,
  `UIManager` (constructor params and member refs)
- InputHandler now holds only `EventBus&`, config constants, and FSM state
- **Verify**: build + tests pass, app runs identically

### Step 5: Create App class, extract wiring from main.cpp

- Create `src/app/App.h` + `src/app/App.cpp`
- Move all object construction from `main.cpp` into `App::Init()`
- Move event wiring (On/Subscribe calls) into `App::WireEvents()`
- Move frame loop into `App::Run()`
- `main.cpp` becomes ~5 lines
- **Verify**: build + tests pass, app runs identically

### Step 6: Strip UIManager down to UI-only

- Remove `DocumentManager&`, `LayoutEngine&`, `Renderer&` from UIManager constructor
- `ChangeFontSize()` now emits `FontSizeEvent` instead of calling 4 modules directly
- `SwitchSource()` emits `SourceSwitchEvent` instead of orchestrating the switch
- Font size/source switch orchestration that touches multiple modules moves to
  `App::WireEvents()` (where it's visible and central)
- **Verify**: build + tests pass, app runs identically

### Step 7: Add namespaces (mechanical)

- Wrap every header's declarations in `namespace theword::<module> { ... }`
- Add `namespace theword::<module> { }` closing at end of each header
- Update all cross-module references to use qualified names
- `using namespace theword::<module>` at top of `.cpp` files where convenient
- **Verify**: build + tests pass, app runs identically

### Step 8: Platform extensions (optional, independent)

- Add `platform::OpenURL()` — wire to JNI on Android, `xdg-open` on Linux
- Add `platform::WriteLog()` — move `#ifdef __ANDROID__` from `Logger.cpp` into
  platform dispatch
- **Verify**: build + tests pass, log output unchanged

---

## Appendix A — Event Flow Diagram

```
Frame N:
  InputHandler::Poll()
    ├─ Emit(ScrollEvent{delta})
    ├─ Emit(SelectionEvent{Start, x, y})
    ├─ Emit(KeyEvent{key::ESCAPE})
    └─ Emit(ResizeEvent{w, h, prevY})

  EventBus dispatch (process all queued events):
    ├─ ScrollEvent → DocumentManager::OnScroll
    ├─ SelectionEvent → Highlighter::OnSelection
    ├─ KeyEvent → UIManager::OnKey → Emit(DialogEvent{GoTo, Toggle})
    ├─ ResizeEvent → LayoutEngine::OnResize
    ├─ ResizeEvent → DocumentManager::OnResize
    └─ DialogEvent → App handles routing (next frame InputHandler checks active dialog)

  DocumentManager::Update(dt):
    └─ Smooth scroll interpolation (no events, internal state)

  App::Draw():
    ├─ Renderer::DrawFrame(...)      ← reads doc spans via App
    ├─ UIManager::DrawTopBar(...)
    ├─ UIManager::DrawDialogs(...)
    └─ EndDrawing()
```

## Appendix B — Files to create or modify

| Step | Files created | Files modified |
|------|---------------|----------------|
| 1 | `src/event/EventBus.h`, `src/event/Events.h` | `CMakeLists.txt` (new directory) |
| 2 | — | `src/input/InputHandler.h`, `src/input/InputHandler.cpp` |
| 3 | — | `src/document/DocumentManager.h/.cpp`, `src/highlight/Highlighter.h/.cpp`, `src/text/LayoutEngine.h/.cpp`, `src/renderer/Renderer.h/.cpp`, `src/renderer/UIManager.h/.cpp` |
| 4 | — | `src/input/InputHandler.h/.cpp` (remove deps) |
| 5 | `src/app/App.h`, `src/app/App.cpp` | `src/main.cpp`, `CMakeLists.txt` |
| 6 | — | `src/renderer/UIManager.h/.cpp`, `src/app/App.cpp` (WireEvents) |
| 7 | — | Every `.h` and `.cpp` file under `src/` (mechanical rename) |
| 8 | — | `src/core/Platform.h/.cpp`, `src/core/Logger.cpp` |

Each step is gated by clean build + 64/64 tests passing + manual smoke test.
