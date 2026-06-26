# Re-architecture Migration — Implementation Plan

> Current Phase: **1/6** — Event Infrastructure

---

## Overview

6 sequential phases, each independently verifiable (build + 64 tests + app smoke test).

```
Phase 1 ──→ Phase 2 ──→ Phase 3 ──→ Phase 4 ──→ Phase 5 ──→ Phase 6
  event/     decouple     App class    strip       namespaces   platform
  infra      I/O                      UIMgr                    extensions
```

**Phase 2 is the riskiest** — replaces direct method calls with events across 12+ files.
All other phases are low-risk (new files, pure moves, mechanical renames).

---

## Phase 1 — Event Infrastructure

**Goal**: Create the `event/` module. No behavioral changes to existing code.

### Files to create

#### `src/event/EventBus.h`

```cpp
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

Key design decisions:
- `Slot` takes `const T&` — no copies of the event for subscribers
- `std::shared_ptr` wrapping enables future `Disconnect()` by storing the shared_ptr key
- `std::any_cast<const T&>` preserves const-correctness
- **Not thread-safe** — all events emitted from main thread

#### `src/event/Events.h`

```cpp
#ifndef EVENTS_H
#define EVENTS_H

namespace theword::event {

struct ScrollEvent       { float delta; };

struct SelectionEvent {
    enum class Action { Start, Update, End, Cancel } action;
    int startWordId;
    int endWordId;
};

struct ResizeEvent {
    int width;
    int height;
    float prevScrollY;
};

struct FontSizeEvent     { float newSize; };
struct SourceSwitchEvent { bool online; };

struct DialogEvent {
    enum class Type { GoTo, Settings, About, ContextMenu } type;
    enum class Action { Show, Hide, Toggle } action;
};

struct KeyEvent          { int key; };
struct NavigateEvent     { const char* bookId; int chapter; };

} // namespace theword::event
#endif
```

### Files to modify

#### `CMakeLists.txt`

Add `src/event/` directory. Since `event/` has only headers, list them in the header list:

```cmake
# In the EVENT_HEADERS list (or equivalent):
set(EVENT_HEADERS
    src/event/EventBus.h
    src/event/Events.h
)
```

Headers-only means no `.cpp` files to compile. Add `EVENT_HEADERS` to the header list.

### Verification

```bash
rm -rf build && cmake --preset default && cmake --build build --parallel && ./build/theword_test
```

Expected: 64/64 pass, app runs identically.

---

## Phase 2 — Input-Output Decoupling

**Goal**: EventBus injected into all modules. InputHandler emits events instead of holding module references. Consumers subscribe to events. One-shot change.

### Strategy

One-shot — not dual-path. Rationale:
- Dual-path risks double-execution (e.g., `ScrollBy` called twice per frame)
- EventBus is ~40 lines, easily testable
- 64 tests + manual smoke test catch regressions

### Files to modify

#### `src/input/InputHandler.h`

Remove member references for `DocumentManager`, `Highlighter`, `LayoutEngine`, `UIManager`.
Add `EventBus&` and `bool dialogActive_`.

```cpp
// Before:
class InputHandler {
public:
    InputHandler(DocumentManager& docManager, Highlighter& highlighter,
                 LayoutEngine& layoutEngine, UIManager& uiManager,
                 float contentTop, float scale = 1.0f);
    void HandleInput(float deltaTime);
private:
    DocumentManager& docManager_;
    Highlighter& highlighter_;
    LayoutEngine& layoutEngine_;
    UIManager& uiManager_;
    // ... FSM state
};

// After:
class InputHandler {
public:
    InputHandler(EventBus& eventBus, float contentTop, float scale = 1.0f);
    void Poll(float deltaTime);
    bool IsDialogActive() const { return dialogActive_; }
private:
    EventBus& eventBus_;
    bool dialogActive_ = false;
    // ... same FSM state
};
```

#### `src/input/InputHandler.cpp`

Every method call on `docManager_`, `highlighter_`, `layoutEngine_`, `uiManager_` replaced with `eventBus_.Emit(...)`.

**Scroll**:
```cpp
// Before:
docManager_.ScrollBy(delta);

// After:
eventBus_.Emit(ScrollEvent{delta});
```

**Selection**:
```cpp
// Before:
highlighter_.StartSelection(startWord, endWord);
// After:
eventBus_.Emit(SelectionEvent{SelectionEvent::Action::Start, startWord, endWord});
```

**Escape key** (dismiss dialog):
```cpp
// Before:
if (uiManager_.IsDialogActive()) {
    uiManager_.DismissActiveDialog();
    return;
}
// After:
if (dialogActive_) {
    eventBus_.Emit(KeyEvent{key::ESCAPE});
    return;
}
```

**Window resize**:
```cpp
// Before:
layoutEngine_.SetMaxWidth(newWidth);
layoutEngine_.InvalidateCache();
docManager_.SetViewportHeight(newHeight);
docManager_.InvalidateLayouts();
docManager_.ScrollTo(scrollFraction * newTotalHeight);

// After:
eventBus_.Emit(ResizeEvent{newWidth, newHeight, currentScrollY});
```

**Keyboard shortcuts (G, S, A)**:
```cpp
// Before:
uiManager_.ToggleGoToDialog();
// After:
eventBus_.Emit(DialogEvent{DialogEvent::Type::GoTo, DialogEvent::Action::Toggle});
```

**Dialog active state tracking**: InputHandler subscribes to `DialogEvent` to update its local flag:

```cpp
// In constructor:
eventBus_.On<DialogEvent>([this](const DialogEvent& e) {
    dialogActive_ = (e.action != DialogEvent::Action::Hide);
});
```

**Context menu state**: InputHandler currently checks `uiManager_.contextMenu->IsVisible()`. Replace with a local flag updated via `DialogEvent`.

**Touch/pinch handling**: Same pattern — emit events instead of calling methods.

#### `src/document/DocumentManager.h`

Add `EventBus&` to constructor. Add subscription methods.

```cpp
// Before:
class DocumentManager {
public:
    DocumentManager(LayoutEngine& layoutEngine, ChapterProvider& provider,
                    float viewportHeight, float contentTop);
    void ScrollBy(float delta);
    void ScrollTo(float y);
    void InvalidateLayouts();
    void SetViewportHeight(float h);
    void SetContentTop(float top);
    void SetFontSize(float size);
    // ...
};

// After:
class DocumentManager {
public:
    DocumentManager(EventBus& eventBus, LayoutEngine& layoutEngine,
                    ChapterProvider& provider, float viewportHeight, float contentTop);
    // Public methods kept for App direct calls during initial load:
    void LoadInitialChapter(const char* bookId, int chapter);
    // Internal event handlers:
    void OnScroll(const ScrollEvent& e);
    void OnResize(const ResizeEvent& e);
    void OnFontSize(const FontSizeEvent& e);
    void OnSourceSwitch(const SourceSwitchEvent& e);
    // Read-only accessors unchanged:
    void GetVisibleSpans(...);
    int HitTestWord(...);
    float GetScrollY() const;
    // ...
private:
    EventBus& eventBus_;
    // ...
};
```

#### `src/document/DocumentManager.cpp`

Subscriptions in constructor:
```cpp
DocumentManager::DocumentManager(EventBus& eventBus, LayoutEngine& layoutEngine,
                                 ChapterProvider& provider, float viewportHeight,
                                 float contentTop)
    : eventBus_(eventBus), layoutEngine_(layoutEngine), provider_(provider), ... {
    eventBus_.On<ScrollEvent>([this](const auto& e) { OnScroll(e); });
    eventBus_.On<ResizeEvent>([this](const auto& e) { OnResize(e); });
    eventBus_.On<FontSizeEvent>([this](const auto& e) { OnFontSize(e); });
    eventBus_.On<SourceSwitchEvent>([this](const auto& e) { OnSourceSwitch(e); });
}
```

Event handlers call old private methods:
```cpp
void DocumentManager::OnScroll(const ScrollEvent& e) {
    scrollBy(e.delta);  // old private implementation
}

void DocumentManager::OnResize(const ResizeEvent& e) {
    layoutEngine_.SetMaxWidth(e.width - 2 * MARGIN);
    layoutEngine_.InvalidateCache();
    viewportHeight_ = e.height;
    invalidateLayouts();
    float newTotal = computeTotalHeight();
    scrollTo(e.prevScrollY / newTotal * newTotal);
}

void DocumentManager::OnFontSize(const FontSizeEvent& e) {
    invalidateLayouts();
}

void DocumentManager::OnSourceSwitch(const SourceSwitchEvent& e) {
    // Reload current chapter — called after CompositeProvider switches
    // (orchestration in App::WireEvents, but DocumentManager just reloads when told)
    auto ref = getCurrentRef();
    LoadChapter(ref.bookId, ref.chapter);
}
```

#### `src/highlight/Highlighter.h`

```cpp
// Before:
class Highlighter {
public:
    Highlighter(PersistenceInterface& storage);
    void StartSelection(int wordId, int chapterId);
    void UpdateSelection(int wordId);
    int EndSelection();
    void CancelSelection();
    // ...
};

// After:
class Highlighter {
public:
    Highlighter(EventBus& eventBus, PersistenceInterface& storage);
    void OnSelection(const SelectionEvent& e);
    // ... (read-only methods unchanged)
private:
    EventBus& eventBus_;
    // ...
};
```

#### `src/highlight/Highlighter.cpp`

```cpp
Highlighter::Highlighter(EventBus& eventBus, PersistenceInterface& storage)
    : eventBus_(eventBus), storage_(storage) {
    eventBus_.On<SelectionEvent>([this](const auto& e) { OnSelection(e); });
}

void Highlighter::OnSelection(const SelectionEvent& e) {
    switch (e.action) {
        case SelectionEvent::Action::Start:  startSelection(e.startWordId, chapterId_); break;
        case SelectionEvent::Action::Update: updateSelection(e.endWordId); break;
        case SelectionEvent::Action::End:    endSelection(); break;
        case SelectionEvent::Action::Cancel: cancelSelection(); break;
    }
}
```

#### `src/text/LayoutEngine.h`

```cpp
// Before:
class LayoutEngine {
public:
    LayoutEngine(float maxWidth, Font bodyFont, float fontSize, ...);
    void SetMaxWidth(float w);
    void SetFontSize(float size);
    void InvalidateCache();
    ChapterLayout LayoutChapter(const ChapterData& data);
    // ...
};

// After:
class LayoutEngine {
public:
    LayoutEngine(EventBus& eventBus, float maxWidth, Font bodyFont, float fontSize, ...);
    void OnResize(const ResizeEvent& e);
    void OnFontSize(const FontSizeEvent& e);
    // LayoutChapter is read-only — unchanged
    ChapterLayout LayoutChapter(const ChapterData& data);
    // ...
private:
    EventBus& eventBus_;
    // ...
};
```

#### `src/text/LayoutEngine.cpp`

```cpp
LayoutEngine::LayoutEngine(EventBus& eventBus, ...)
    : eventBus_(eventBus), ... {
    eventBus_.On<ResizeEvent>([this](const auto& e) { OnResize(e); });
    eventBus_.On<FontSizeEvent>([this](const auto& e) { OnFontSize(e); });
}

void LayoutEngine::OnResize(const ResizeEvent& e) {
    SetMaxWidth(e.width - 2 * MARGIN);
    InvalidateCache();
}

void LayoutEngine::OnFontSize(const FontSizeEvent& e) {
    SetFontSize(e.newSize);
    InvalidateCache();
}
```

#### `src/renderer/Renderer.h`

```cpp
// Before:
class Renderer {
public:
    Renderer(Font bodyFont, Font headingFont, float contentTop, float fontSize);
    // ...
    void SetFontSize(float size);
    // ...
};

// After:
class Renderer {
public:
    Renderer(EventBus& eventBus, Font bodyFont, Font headingFont, float contentTop, float fontSize);
    void OnFontSize(const FontSizeEvent& e);
    // ...
private:
    EventBus& eventBus_;
    // ...
};
```

#### `src/renderer/Renderer.cpp`

```cpp
Renderer::Renderer(EventBus& eventBus, ...)
    : eventBus_(eventBus), ... {
    eventBus_.On<FontSizeEvent>([this](const auto& e) { OnFontSize(e); });
}

void Renderer::OnFontSize(const FontSizeEvent& e) {
    fontSize_ = e.newSize;
    headingSize_ = e.newSize * theme::FONT_HEADING;
}
```

#### `src/renderer/UIManager.h`

UIManager becomes both an emitter and subscriber. It still holds DocumentManager/LayoutEngine/Renderer refs for now (they're removed in Phase 4). For Phase 2, it adds EventBus subscription and emission alongside existing paths.

```cpp
// Before:
class UIManager {
public:
    UIManager(DocumentManager& docManager, Highlighter& highlighter, ...);
    void HideContextMenu();
    void ShowContextMenu(int wordId, float x, float y);
    void ToggleGoToDialog();
    void ToggleSettings();
    void ToggleAbout();
    bool IsDialogActive() const;
    void DismissActiveDialog();
    void ChangeFontSize(float newSize);
    void SwitchSource(bool online);
    void DrawTopBar(const char* title);
    void DrawContextMenu();
    void DrawGoToDialog();
    void DrawSettingsPanel();
    void DrawAbout();
    // ...
};

// After:
class UIManager {
public:
    UIManager(EventBus& eventBus, DocumentManager& docManager, Highlighter& highlighter, ...);
    void OnKey(const KeyEvent& e);
    void OnDialog(const DialogEvent& e);
    // Draw methods unchanged
    // ChangeFontSize/SwitchSource still call old methods AND emit events (dual-path for Phase 2)
    // ...
};
```

#### `src/renderer/UIManager.cpp`

```cpp
UIManager::UIManager(EventBus& eventBus, ...)
    : eventBus_(eventBus), ... {
    eventBus_.On<KeyEvent>([this](const auto& e) { OnKey(e); });
    eventBus_.On<DialogEvent>([this](const auto& e) { OnDialog(e); });
}

void UIManager::OnKey(const KeyEvent& e) {
    if (e.key == key::ESCAPE) DismissActiveDialog();
    else if (e.key == key::G) ToggleGoToDialog();
    else if (e.key == key::S) ToggleSettings();
    else if (e.key == key::A) ToggleAbout();
    else if (activeDialog_ && activeDialog_->HandleKey(e.key)) { }
}

void UIManager::OnDialog(const DialogEvent& e) {
    switch (e.type) {
        case DialogEvent::Type::GoTo:       ToggleGoToDialog(); break;
        case DialogEvent::Type::Settings:   ToggleSettings(); break;
        case DialogEvent::Type::About:      ToggleAbout(); break;
        case DialogEvent::Type::ContextMenu: HideContextMenu(); break;
    }
}

void UIManager::ChangeFontSize(float newSize) {
    // Dual-path for Phase 2: emit event AND call old methods
    eventBus_.Emit(FontSizeEvent{newSize});
    // old direct calls kept for now — will be removed in Phase 4
    docManager_.InvalidateLayouts();
    renderer_.SetFontSize(newSize);
    layoutEngine_.SetFontSize(newSize);
    layoutEngine_.InvalidateCache();
    currentFontSize_ = newSize;
    persistence_.SavePreference("font_size", std::to_string((int)newSize));
}
```

#### `src/main.cpp`

```cpp
// Before:
auto main() -> int {
    auto plat = platform::Init("TheWord");
    // ... all construction ...
    InputHandler inputHandler(docManager, highlighter, layoutEngine, uiManager, ...);
    // ... loop ...
}

// After:
auto main() -> int {
    auto plat = platform::Init("TheWord");

    // Event bus — created early, passed everywhere
    theword::event::EventBus eventBus;

    // Same construction, but EventBus passed as first arg:
    USFMParser usfmParser(USFM_DIR, plat.assets.get());
    std::unique_ptr<BibleClient> bibleClient;
    std::unique_ptr<CompositeProvider> compositeProv;
    if (apiClient) {
        bibleClient = std::make_unique<BibleClient>(*apiClient);
        compositeProv = std::make_unique<CompositeProvider>(*bibleClient, usfmParser);
    }
    PersistenceManager persistence(plat.dbPath);
    Highlighter highlighter(eventBus, persistence);
    LayoutEngine layoutEngine(eventBus, contentWidth, bodyFont, fontSize, ...);
    Renderer renderer(eventBus, bodyFont, headingFont, contentTop, fontSize);
    DocumentManager docManager(eventBus, layoutEngine,
        compositeProv ? *compositeProv : usfmParser, viewportHeight, contentTop);
    UIManager uiManager(eventBus, docManager, highlighter, layoutEngine, renderer,
        persistence, *onlineProv, *offlineProv, compositeProv.get(), ...);
    InputHandler inputHandler(eventBus, contentTop, scale);

    docManager.LoadInitialChapter("GEN.1");

    while (!platform::ShouldQuit()) {
        float dt = GetFrameTime();
        inputHandler.Poll(dt);
        // DocumentManager still called directly for Update (smooth scroll)
        docManager.Update(dt);
        // Draw unchanged
        BeginDrawing();
        ClearBackground(theme::COLOR_BACKGROUND);
        // ...
    }
    // cleanup
}
```

### Potential pitfalls

1. **dialogActive_ race**: InputHandler checks `dialogActive_` to suppress scroll/selection while a dialog is open. If the `DialogEvent` subscription updates the flag one frame late, input bleeds through for one frame. Fix: emit `DialogEvent` synchronously before InputHandler checks the flag. Since EventBus dispatches synchronously in `Emit()`, the flag updates before the next `Poll()` call.

2. **Resize double-handling**: Both InputHandler (old path) and LayoutEngine/DocumentManager (via event) handle resize. In Phase 2, InputHandler no longer handles resize directly — only the event subscribers do. This is correct.

3. **Context menu visibility**: InputHandler checks `uiManager_.contextMenu->IsVisible()` to suppress selection during context menu. After decoupling, maintain a local `contextMenuVisible_` flag updated via `DialogEvent{ContextMenu, Show/Hide}`.

4. **Highlight orphan on source switch**: When source switches, highlights tagged with the old provider are hidden. This is already handled in existing code (provider_name column). No event-related concern.

### Verification

```bash
rm -rf build && cmake --preset default && cmake --build build --parallel && ./build/theword_test
# Manual: run app, test scroll, click, resize, G/S/A shortcuts, highlight selection
```

---

## Phase 3 — App Class Extraction

**Goal**: Create `src/app/App.h/.cpp`. Move all wiring from `main.cpp` into `App::Init()` and `App::WireEvents()`. `main.cpp` becomes 5 lines.

### Files to create

#### `src/app/App.h`

```cpp
#ifndef APP_H
#define APP_H

#include <memory>
#include <string>

namespace theword::core     { class Platform; }
namespace theword::event    { class EventBus; }
namespace theword::data     { class USFMParser; class BibleClient; class CompositeProvider; }
namespace theword::persistence { class PersistenceManager; }
namespace theword::highlight   { class Highlighter; }
namespace theword::text     { class LayoutEngine; }
namespace theword::document { class DocumentManager; }
namespace theword::renderer { class Renderer; class UIManager; }
namespace theword::input    { class InputHandler; }

namespace theword::app {

class App {
public:
    App();
    ~App();
    bool Init(const std::string& title);
    void Run();
    bool ShouldQuit() const;

private:
    void WireEvents();

    // Platform info — plain struct, not a pointer
    struct PlatformInfo {
        std::string dbPath;
        float dpiScale;
    };
    PlatformInfo plat_;

    // Fonts loaded once, stored for lifetime
    Font bodyFont_;
    Font headingFont_;

    // Owned subsystems
    std::unique_ptr<event::EventBus>          eventBus_;
    std::unique_ptr<data::USFMParser>         usfmParser_;
    std::unique_ptr<data::BibleClient>        bibleClient_;
    std::unique_ptr<data::CompositeProvider>  compositeProv_;
    std::unique_ptr<persistence::PersistenceManager> persistence_;
    std::unique_ptr<hierarchy::Highlighter>   highlighter_;
    std::unique_ptr<text::LayoutEngine>       layoutEngine_;
    std::unique_ptr<renderer::Renderer>       renderer_;
    std::unique_ptr<document::DocumentManager> docManager_;
    std::unique_ptr<renderer::UIManager>      uiManager_;
    std::unique_ptr<input::InputHandler>      inputHandler_;

    // Active provider (CompositeProvider if API available, otherwise USFMParser)
    data::ChapterProvider* activeProv_ = nullptr;
    data::ChapterProvider* onlineProv_ = nullptr;
    data::ChapterProvider* offlineProv_ = nullptr;
};

} // namespace theword::app
#endif
```

#### `src/app/App.cpp`

`Init()` contains the exact same wiring as current `main.cpp`, moved verbatim:

```cpp
#include "app/App.h"

namespace theword::app {

App::App() = default;
App::~App() = default;

bool App::Init(const std::string& title) {
    auto info = platform::Init(title.c_str());
    plat_.dbPath = info.dbPath;
    plat_.dpiScale = info.dpiScale;
    SetTargetFPS(config::TARGET_FPS);

    // Draw splash before fonts
    BeginDrawing();
    ClearBackground(theme::COLOR_BACKGROUND);
    // ... splash drawing ...
    EndDrawing();

    // Load fonts through asset provider
    bodyFont_ = LoadFontCodepoints(*info.assets, config::FONT_REGULAR);
    headingFont_ = LoadFontEx(config::FONT_BOLD, ...);

    // Compute layout constants
    float contentWidth = ...;
    float contentTop = ...;
    float headingSize = ...;
    float fontSize = ...;
    float scale = plat_.dpiScale;

    // API key
    EnvLoader envLoader;
    envLoader.Load();
    std::string apiKey = envLoader.Get("YVP_APP_KEY");
    std::unique_ptr<core::IHttpClient> apiClient = platform::CreateHttpClient();

    // Create subsystems
    eventBus_ = std::make_unique<event::EventBus>();
    usfmParser_ = std::make_unique<data::USFMParser>(config::USFM_DIR, info.assets.get());
    offlineProv_ = usfmParser_.get();

    if (apiClient && !apiKey.empty()) {
        // ... setup BibleClient + CompositeProvider ...
    } else {
        activeProv_ = offlineProv_;
    }

    persistence_ = std::make_unique<persistence::PersistenceManager>(plat_.dbPath);
    // Load saved preferences
    float savedFontSize = ...;
    // ...

    highlighter_ = std::make_unique<hierarchy::Highlighter>(*eventBus_, *persistence_);
    layoutEngine_ = std::make_unique<text::LayoutEngine>(*eventBus_, contentWidth, bodyFont_, savedFontSize, ...);
    renderer_ = std::make_unique<renderer::Renderer>(*eventBus_, bodyFont_, headingFont_, contentTop, savedFontSize);
    docManager_ = std::make_unique<document::DocumentManager>(*eventBus_, *layoutEngine_,
        *activeProv_, viewportHeight, contentTop);
    uiManager_ = std::make_unique<renderer::UIManager>(*eventBus_, *docManager_, *highlighter_,
        *layoutEngine_, *renderer_, *persistence_, ...);
    inputHandler_ = std::make_unique<input::InputHandler>(*eventBus_, contentTop, scale);

    // Wire events
    WireEvents();

    // Load initial chapter
    docManager_->LoadInitialChapter("GEN.1");

    return true;
}

void App::WireEvents() {
    // Cross-cutting event orchestration lives here.
    // Currently empty — filled in during Phase 4.
    // Phase 2 subscriptions happen in each subsystem's constructor.
}

void App::Run() {
    while (!ShouldQuit()) {
        float dt = GetFrameTime();
        inputHandler_->Poll(dt);
        docManager_->Update(dt);

        BeginDrawing();
        ClearBackground(theme::COLOR_BACKGROUND);

        // ... same draw order as current main.cpp ...
        // Gather spans, build highlight rects, draw frame
        std::vector<std::pair<Span, float>> docSpans;
        docManager_->GetVisibleSpans(docSpans, ...);
        std::vector<HighlightRect> hlRects;
        // build hlRects from highlighter
        renderer_->DrawFrame(scrollY, totalHeight, viewportHeight, docSpans, hlRects);
        uiManager_->DrawContextMenu();
        uiManager_->DrawGoToDialog();
        uiManager_->DrawSettingsPanel();
        uiManager_->DrawAbout();
        if constexpr (!NDEBUG) renderer_->DrawFpsCounter(...);

        EndDrawing();
    }
}

bool App::ShouldQuit() const {
    return platform::ShouldQuit() || WindowShouldClose();
}

} // namespace theword::app
```

#### `src/main.cpp`

```cpp
#include "app/App.h"

auto main() -> int {
    theword::app::App app;
    if (!app.Init("TheWord")) return 1;
    app.Run();
    return 0;
}
```

### Files to modify

#### `CMakeLists.txt`

Add `src/app/App.h` and `src/app/App.cpp` to source lists.

### Verification

```bash
cmake --build build --parallel && ./build/theword_test
# Manual: app runs identically
```

---

## Phase 4 — UIManager Refinement

**Goal**: Remove `DocumentManager&`, `LayoutEngine&`, `Renderer&` from UIManager. These cross-cutting concerns move to `App::WireEvents()`.

### Files to modify

#### `src/renderer/UIManager.h`

Remove `DocumentManager&`, `LayoutEngine&`, `Renderer&` member refs.

```cpp
class UIManager {
public:
    UIManager(EventBus& eventBus, Highlighter& highlighter,
              PersistenceManager& persistence, CompositeProvider* compositeProv,
              float headingSize, float scale);
    void OnFontSizeApplied(float newSize);  // UI-only: update label
    // ...
private:
    EventBus& eventBus_;
    Highlighter& highlighter_;
    PersistenceManager& persistence_;
    CompositeProvider* compositeProv_;
    float headingSize_;
    float scale_;
    float currentFontSize_;
    bool versionOnline_;
    // Sub-dialogs (owned)
    std::unique_ptr<ContextMenu> contextMenu_;
    std::unique_ptr<SettingsPanel> settingsPanel_;
    std::unique_ptr<GoToDialog> goToDialog_;
    std::unique_ptr<AboutOverlay> aboutOverlay_;
};
```

#### `src/renderer/UIManager.cpp`

**ChangeFontSize** — emit event only, no direct calls:
```cpp
void UIManager::ChangeFontSize(float newSize) {
    eventBus_.Emit(FontSizeEvent{newSize});
    currentFontSize_ = newSize;
    persistence_.SavePreference("font_size", std::to_string((int)newSize));
}

void UIManager::OnFontSizeApplied(float newSize) {
    // Called by App after all subscribers have processed the event
    // Refreshes the font size display in SettingsPanel (if visible)
    currentFontSize_ = newSize;
}
```

**SwitchSource** — emit event only:
```cpp
void UIManager::SwitchSource(bool online) {
    versionOnline_ = online;
    eventBus_.Emit(SourceSwitchEvent{online});
    persistence_.SavePreference("active_version", online ? "1" : "0");
}
```

**ApplyFontSize** — removed (handled by event subscribers).

#### `src/app/App.cpp` — `WireEvents()`

The cross-cutting orchestration moves here:

```cpp
void App::WireEvents() {
    eventBus_->On<FontSizeEvent>([this](const FontSizeEvent& e) {
        // Order matters: set size → invalidate → re-layout → update display
        layoutEngine_->SetFontSize(e.newSize);
        layoutEngine_->InvalidateCache();
        docManager_->InvalidateLayouts();
        renderer_->SetFontSize(e.newSize);
        uiManager_->OnFontSizeApplied(e.newSize);
    });

    eventBus_->On<SourceSwitchEvent>([this](const SourceSwitchEvent& e) {
        compositeProv_->SetPrimary(e.online ? 0 : 1);
        // Reload current chapter from new source
        auto ref = docManager_->GetCurrentChapterRef();
        docManager_->ReloadChapter(ref.bookId, ref.chapter);
    });
}
```

### Verification

```bash
cmake --build build --parallel && ./build/theword_test
# Manual: font size A–/A+, source toggle, app runs identically
```

---

## Phase 5 — Namespace Migration

**Goal**: Every class under `theword::<module>::` namespace.

### Strategy

Mechanical, one file at a time. No logic changes. Order by dependency (core first):

1. `core/` (no deps on other modules)
2. `data/` (depends on core)
3. `text/` (depends on data)
4. `document/` (depends on text + data)
5. `highlight/` (no deps on other modules beyond core)
6. `persistence/` (depends on highlight)
7. `renderer/` (depends on data + core + document)
8. `input/` (depends on core only after decoupling)
9. `event/` (already namespaced)
10. `app/` (depends on everything)
11. `main.cpp` (uses fully qualified names)
12. `tests/` (uses fully qualified names)

### Pattern per file

**Header**:
```cpp
#ifndef LAYOUTENGINE_H
#define LAYOUTENGINE_H

// ... includes ...

namespace theword::text {
class LayoutEngine {
    // ... unchanged ...
};
} // namespace theword::text
#endif
```

**Source**:
```cpp
#include "text/LayoutEngine.h"
#include "core/Theme.h"

namespace theword::text {

LayoutEngine::LayoutEngine(...) : ... { }

// Methods unchanged

} // namespace theword::text
```

**Cross-module references** in headers get qualified:
```cpp
// Before:
class DocumentManager {
    DocumentManager(LayoutEngine& layoutEngine, ChapterProvider& provider, ...);
};

// After:
namespace theword::document {
class DocumentManager {
    DocumentManager(theword::text::LayoutEngine& layoutEngine,
                    theword::data::ChapterProvider& provider, ...);
};
} // namespace theword::document
```

**In .cpp files**, a single `using` at the top for own module:
```cpp
#include "document/DocumentManager.h"
#include "core/BibleBooks.h"
#include "core/Theme.h"

namespace theword::document {
    using namespace theword::core;     // for config::, key::, theme::
    using namespace theword::text;     // for LayoutEngine
    using namespace theword::data;     // for ChapterProvider, etc.
    // ... implementation ...
} // namespace theword::document
```

### Files to skip

- `src/core/Config.h` — already has `config` and `key` namespaces. Wrap them in `namespace theword::core { namespace config { ... } namespace key { ... } }`.
- `src/core/Theme.h` — already has `theme` namespace. Wrap in `namespace theword::core { namespace theme { ... } }`.
- `src/event/` — already created with namespaces in Phase 1.
- `src/main/java/` — C++ namespaces don't apply.

### Verification

```bash
cmake --build build --parallel && ./build/theword_test
# Manual: app runs identically
```

---

## Phase 6 — Platform Extensions

**Goal**: Add `platform::OpenURL()`, abstracted log sink, clipboard.

Independent of all other phases — can be done last.

### Files to modify

#### `src/core/Platform.h`

```cpp
namespace theword::core::platform {

struct Info { /* unchanged */ };
Info Init(const char* title);
std::unique_ptr<IHttpClient> CreateHttpClient();
bool ShouldQuit();
void OpenURL(const char* url);
std::string GetClipboard();
void SetClipboard(const std::string& text);

enum class LogLevel { DEBUG, INFO, WARN, ERROR };
void WriteLog(LogLevel level, const char* message);

} // namespace
```

#### `src/core/Platform.cpp`

```cpp
void OpenURL(const char* url) {
#if defined(__linux__)
    std::string cmd = "xdg-open ";
    cmd += url;
    system(cmd.c_str());
#elif defined(_WIN32)
    ShellExecute(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);
#elif defined(__APPLE__)
    std::string cmd = "open ";
    cmd += url;
    system(cmd.c_str());
#elif defined(__ANDROID__)
    // JNI call to ACTION_VIEW intent
    // ...
#endif
}

std::string GetClipboard() {
#ifdef __ANDROID__
    // JNI call to ClipboardManager
    // ...
#else
    return GetClipboardText();
#endif
}

void SetClipboard(const std::string& text) {
#ifdef __ANDROID__
    // JNI call
    // ...
#else
    SetClipboardText(text.c_str());
#endif
}

void WriteLog(LogLevel level, const char* message) {
#ifdef __ANDROID__
    android_LogPriority prio = ANDROID_LOG_DEBUG;
    switch (level) {
        case LogLevel::INFO:  prio = ANDROID_LOG_INFO;  break;
        case LogLevel::WARN:  prio = ANDROID_LOG_WARN;  break;
        case LogLevel::ERROR: prio = ANDROID_LOG_ERROR; break;
        default: break;
    }
    __android_log_write(prio, "TheWord", message);
#else
    FILE* out = (level == LogLevel::ERROR) ? stderr : stdout;
    fprintf(out, "[%s] %s\n", levelName(level), message);
#endif
}
```

#### `src/core/Logger.cpp`

Replace `#ifdef __ANDROID__` blocks:

```cpp
// Before (5 scattered #ifdef blocks):
void Logger::Info(const char* msg) {
#ifdef __ANDROID__
    __android_log_write(ANDROID_LOG_INFO, "TheWord", msg);
#else
    fprintf(stdout, "[INFO] %s\n", msg);
#endif
}

// After:
void Logger::Info(const char* msg) {
    platform::WriteLog(platform::LogLevel::INFO, msg);
}
```

### Verification

```bash
cmake --build build --parallel && ./build/theword_test && ./build/theword
```

---

## Appendix A — File change inventory

| Phase | File | Action |
|-------|------|--------|
| 1 | `src/event/EventBus.h` | Create |
| 1 | `src/event/Events.h` | Create |
| 1 | `CMakeLists.txt` | Add event/ headers |
| 2 | `src/input/InputHandler.h` | Remove 4 refs, add EventBus&, add dialogActive_ |
| 2 | `src/input/InputHandler.cpp` | Replace direct calls with event emissions |
| 2 | `src/document/DocumentManager.h` | Add EventBus&, add On* methods |
| 2 | `src/document/DocumentManager.cpp` | Add subscriptions, event handlers |
| 2 | `src/highlight/Highlighter.h` | Add EventBus&, replace Start/Update/End with OnSelection |
| 2 | `src/highlight/Highlighter.cpp` | Add subscription, delegate to old impl |
| 2 | `src/text/LayoutEngine.h` | Add EventBus&, add OnResize/OnFontSize |
| 2 | `src/text/LayoutEngine.cpp` | Add subscriptions, event handlers |
| 2 | `src/renderer/Renderer.h` | Add EventBus&, add OnFontSize |
| 2 | `src/renderer/Renderer.cpp` | Add subscription, event handler |
| 2 | `src/renderer/UIManager.h` | Add EventBus&, add OnKey/OnDialog |
| 2 | `src/renderer/UIManager.cpp` | Add subscriptions, event handlers, dual-path ChangeFontSize |
| 2 | `src/main.cpp` | Create EventBus, inject into all constructors |
| 3 | `src/app/App.h` | Create |
| 3 | `src/app/App.cpp` | Create (move wiring from main.cpp) |
| 3 | `src/main.cpp` | Reduce to 5 lines |
| 3 | `CMakeLists.txt` | Add app/ sources |
| 4 | `src/renderer/UIManager.h` | Remove DocumentManager&, LayoutEngine&, Renderer& |
| 4 | `src/renderer/UIManager.cpp` | ChangeFontSize/SwitchSource emit-only |
| 4 | `src/app/App.cpp` | WireEvents fills in FontSizeEvent/SourceSwitchEvent orchestration |
| 5 | All ~35 .h/.cpp files under src/ | Add namespace wrapping |
| 6 | `src/core/Platform.h` | Add OpenURL, GetClipboard, SetClipboard, WriteLog |
| 6 | `src/core/Platform.cpp` | Implement above |
| 6 | `src/core/Logger.cpp` | Replace #ifdef with platform::WriteLog |

## Appendix B — Rollback strategy

| Phase | Rollback |
|-------|----------|
| 1 | Delete `src/event/` directory, revert CMakeLists.txt |
| 2 | `git checkout HEAD -- src/ src/main.cpp` — largest rollback, but all changes in src/ |
| 3 | Delete `src/app/`, restore `main.cpp` from git |
| 4 | `git checkout HEAD -- src/renderer/UIManager.* src/app/App.cpp` |
| 5 | `git checkout HEAD -- src/` — touches everything, but pure mechanical |
| 6 | `git checkout HEAD -- src/core/Platform.* src/core/Logger.cpp` |

Always commit after each phase to keep rollback points clean.
