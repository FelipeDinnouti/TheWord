# Architecture Analysis

> Generated: 2026-07-18 | Context: v1.8.0-alpha Code Quality Audit follow-up
> 
> **Immediate remediation (A1, A3, A6, A7, A8) completed 2026-07-18.**
> **Remaining (A2, A4, A5, A9) deferred to v1.9.0-alpha refactor release.**

## Executive Summary

The Code Quality Audit (Step 1-8) focused on surface-level hygiene: warnings, dead code,
includes, const-correctness, naming. It did not examine **module-to-module** relationships
or compare the actual code against the documented architecture. This document fills that gap.

**9 findings, 3 severity levels:**

| # | Finding | Severity | Effort |
|---|---------|----------|--------|
| A1 | `ui/` module undocumented in architecture docs | ⚪ Documentation | Trivial |
| A2 | Renderer directly includes `data/ChapterProvider.h` (layer skip) | 🔴 Critical | Medium |
| A3 | InputHandler includes NavigationStack (backward dependency) | 🔴 Critical | Small |
| A4 | All `ui/` screens bypass Renderer, call platform APIs directly | 🟠 Major | Large |
| A5 | LayoutEngine depends on raylib `Font` + `MeasureTextEx` | 🟠 Major | Medium |
| A6 | Highlighter depends on raylib `Color` type | 🟠 Major | Small |
| A7 | `#ifdef _WIN32` leak in PersistenceManager | 🟡 Moderate | Trivial |
| A8 | Event bus usage is ad-hoc with no governance | 🟡 Moderate | Small |
| A9 | `App.cpp` is a 757-line god orchestrator | 🟡 Moderate | Large |

Total effort to remediate all 9: **Large** (~3-5 focused sessions).
Effort for Critical only: **Medium** (~1-2 sessions).

---

## Current Architecture: Documented vs Actual

### Documented Layer Diagram (Architecture Overview.md)

```
┌─────────────────────────────────────┐
│           UI LAYER                  │
│  Renderer · InputHandler · UIManager│
├─────────────────────────────────────┤
│         DOCUMENT MANAGER            │
├─────────────────────────────────────┤
│        TEXT LAYOUT ENGINE           │
├─────────────────────────────────────┤
│          DATA LAYER                 │
│  ChapterProvider (interface)        │
│    ├── USFMParser                   │
│    └── BibleClient                  │
│  PersistenceManager (SQLite)        │
└─────────────────────────────────────┘
```

### Actual Module Map

```
src/
├── app/          App.h/cpp            — God orchestrator (757 lines)
├── core/         Config, Theme, Platform, EnvLoader, etc.
├── data/         ChapterProvider, USFMParser, BibleClient, etc.
├── text/         LayoutEngine
├── document/     DocumentManager
├── highlight/    Highlighter, PersistenceInterface
├── persistence/  PersistenceManager
├── input/        InputHandler
├── event/        EventBus, Events
├── renderer/     Renderer, UIManager, RadialMenu
└── ui/           Screen-based UI framework (11 files, NOT in docs)
```

**Key gap: `ui/` is undocumented.** It contains 11 source files implementing a full
screen-based UI framework with its own rendering and input handling, yet the architecture
docs don't mention it. The dependency diagram only shows `Renderer` and `UIManager`.

### Real Dependency Graph (Simplified)

```
                   ┌──────────────────┐
                   │      App         │ ← wires everything, 22+ includes
                   └──┬───┬───┬────┬──┘
                      │   │   │    │
          ┌───────────┘   │   │    └──────────────┐
          ▼               ▼   ▼                   ▼
    ┌──────────┐  ┌──────────┐  ┌───────────┐ ┌──────────┐
    │ Renderer │  │InputHandler│ │UIManager  │ │ ReaderSc │
    │  ┌──────┐│  │  ┌──────┐ │  │┌────────┐│ │ ┌──────┐ │
    │  │Span  ││  │  │NavStk│ │  ││Highlght││ │ │Span  │ │
    │  └──┬───┘│  │  └──┬───┘ │  │└───┬────┘│ │ └──┬───┘ │
    └─────┼────┘  └─────┼─────┘  └────┼─────┘ └────┼─────┘
          │             │             │             │
          ▼             ▼             ▼             ▼
    ┌─────────────────────────────────────────────────────┐
    │               DocumentManager                        │
    │           ┌──────────────────────┐                   │
    │           │    LayoutEngine      │                   │
    │           │  ┌────────────────┐  │                   │
    │           │  │ data/ChapterProv│  │  (Span, ChapterLayout here)
    │           │  └───────┬────────┘  │                   │
    │           └──────────┼───────────┘                   │
    └──────────────────────┼──────────────────────────────┘
                           │
                           ▼
                   ┌──────────────┐
                   │   ChapterProvider (interface)
                   │   ├── USFMParser
                   │   └── BibleClient
                   └──────────────┘
```

**Red arrows** (violations):
- Renderer → data/ChapterProvider (A2)
- InputHandler → ui/NavigationStack (A3)
- ReaderScreen → data/ChapterProvider (A4 variant)
- All ui/ screens → raylib platform APIs (A4)

---

## Findings (A1–A9)

### A1 — `ui/` Module Undocumented

**Files:** `Architecture Overview.md`, `Convention Reference.md`

**Current state:** The `src/ui/` directory contains 11 files implementing a screen-based
UI framework (Screen base class, NavigationStack, 7 screen implementations, TapDetector,
UI components, FontDiagnostic), but it appears in NO architecture diagram.

**Intended state:** Architecture docs must account for `ui/` as a layer that sits
alongside `renderer/` — screens draw themselves and handle their own input.

**Gap:** Missing from layer diagram, dependency graph, module responsibility table,
and documented file tree.

**Remediation:** Update 3 doc files:
- `Architecture Overview.md`: Add `ui/` to layer diagram, add `NavigationStack`, `Screen`
  to dependency graph, add `ui/` entry to module table
- `Convention Reference.md`: Add `ui/` to project structure tree (src/ui/)
- Add note that `ui/` depends on `core/` and `event/`, and is consumed by `app/`

**Effort:** Trivial (~15 min)

---

### A2 — Renderer Includes data/ChapterProvider.h (Layer Skip)

**Files:**
- `src/renderer/Renderer.h:7` — `#include "data/ChapterProvider.h"`
- `src/renderer/Renderer.h:29` — uses `theword::data::Span` in `DrawFrame()` signature
- `src/renderer/Renderer.h:56` — uses `theword::data::Span` in `DrawSpan()` signature
- `src/renderer/Renderer.cpp:38,45,48,56-100` — accesses `span.text`, `span.x`, `span.type`
- `src/data/ChapterProvider.h:43-68` — `Span`, `Line`, `ChapterLayout` defined here

**Current state:** The Renderer directly depends on `data/ChapterProvider.h` to access
the `Span` struct. `Span` contains layout-positioning fields (`x, y, width, height`)
that are produced by LayoutEngine (text module) and consumed by Renderer, but it's
defined in the data layer for historical convenience.

**Intended state:** The acyclic dependency chain is:
`data → text → document → renderer`

The Renderer should never include anything from `data/`. It should consume layout output
types defined in `text/` or `document/`, not in `data/`.

**Why it's a problem:**
1. Violates layering: renderer depends on data layer (2 layers down, skipping text + document)
2. Prevents reorganization: if Span's definition changes for rendering needs, it drags
   in the entire data model
3. Any change to `data/ChapterProvider.h` triggers recompilation of Renderer (unnecessary coupling)

**Remediation:**
1. Extract `Span`, `Line`, `ChapterLayout` from `data/ChapterProvider.h` into a new
   `text/LayoutTypes.h` (shared types header).
2. Have `data/ChapterProvider.h` continue to include `text/LayoutTypes.h` for backwards
   compatibility (or include it inline in the extraction commit).
3. Update `Renderer.h` to include `text/LayoutTypes.h` instead of `data/ChapterProvider.h`.
4. Update `DocumentManager.h` to include `text/LayoutTypes.h` (already includes LayoutEngine.h).
5. Verify: ReaderScreen.cpp also includes `data/ChapterProvider.h` for Span access — this
   would also be fixed by the extraction.
6. Remove direct `ChapterProvider.h` include from `ReaderScreen.cpp`.

**Files to change:**
- Create: `src/text/LayoutTypes.h`
- Modify: `src/renderer/Renderer.h` (change include)
- Modify: `src/renderer/Renderer.cpp` (change using-directive if needed)
- Modify: `src/ui/ReaderScreen.cpp` (change include)
- Modify: `src/data/ChapterProvider.h` (remove Span/Line/ChapterLayout, include LayoutTypes.h)
- No change to LayoutEngine.h — it stays coupled to ChapterProvider.h for Segment/Word types

**Effort:** Medium (~1-2 hours including build-test loop)

---

### A3 — InputHandler Includes NavigationStack (Backward Dependency)

**Files:**
- `src/input/InputHandler.cpp:7` — `#include "ui/NavigationStack.h"`
- `src/input/InputHandler.cpp:52` — `Poll(float deltaTime, NavigationStack* navStack)`
- `src/input/InputHandler.cpp:81-82` — `navStack->HandleInput(deltaTime)`

**Current state:** `InputHandler::Poll()` accepts a `NavigationStack*` parameter and
calls `HandleInput()` on it before running its own FSM. This gives the active screen
first crack at input events. The include creates a backward dependency (input → UI).

**Intended state:** Input should not know about UI screens. The navigation stack should
be consulted by the caller (App), not by InputHandler internally.

**Why it's a problem:**
1. InputHandler is coupled to the screen abstraction — it cannot be tested without a
   NavigationStack
2. The dependency arrow goes against the intended flow (App orchestrates both input and UI)
3. If the screen-based UI framework is ever reorganized, InputHandler must change

**Remediation (option A — using EventBus):**
1. Add `InputAboutToProcessEvent{float deltaTime}` to Events.h.
2. `InputHandler::Poll()` emits `InputAboutToProcessEvent` at the start.
3. `NavigationStack` subscribes and calls `HandleInput()` on the active screen.
4. If screen consumed input, NavigationStack emits `InputConsumedEvent{}`.
5. `InputHandler` checks a flag or returns early.

**Remediation (option B — move delegation to App):**
1. Drop `navStack` parameter from `Poll()`.
2. In `App.cpp`, call `navStack->HandleInput(deltaTime)` **before** `inputHandler.Poll(deltaTime)`.
3. If `HandleInput` returns true, skip `Poll()`.

**Option B is simpler and preferred.** It moves the orchestration to where it belongs:
the App layer.

**Files to change:**
- `src/input/InputHandler.h` — remove `NavigationStack*` param from `Poll()` signature
- `src/input/InputHandler.cpp` — remove include, remove navStack param, remove lines 81-85
- `src/app/App.cpp` — add `if (navStack->HandleInput(deltaTime)) return;` before `inputHandler.Poll()`
- `src/app/App.h` — no change

**Effort:** Small (~30 min)

---

### A4 — UI Screens Bypass Renderer

**Files affected (7 screen files + 1 component file):**

| File | Direct raylib calls |
|------|-------------------|
| `BookListScreen.cpp` | `GetScreenHeight/W`, `GetMousePosition`, `GetMouseWheelMove`, `IsKeyPressed`, `IsMouseButtonPressed/Released` |
| `CenterMenu.cpp` | `GetScreenWidth/H`, `GetMousePosition`, `IsKeyPressed`, `IsMouseButtonPressed/Released` |
| `ChapterGridScreen.cpp` | `GetScreenWidth/H`, `BeginScissorMode/EndScissorMode`, `GetMousePosition`, `GetMouseWheelMove`, `IsKeyPressed`, `IsMouseButtonPressed/Released` |
| `CreditsOverlay.cpp` | `GetScreenWidth/H`, `GetMousePosition`, `IsKeyPressed`, `IsMouseButtonPressed/Released` |
| `FontDiagnostic.cpp` | `GetScreenWidth/H`, `BeginScissorMode/EndScissorMode`, `IsKeyPressed`, `GetMouseWheelMove` |
| `HighlightBrowserScreen.cpp` | `GetScreenWidth/H`, `BeginScissorMode/EndScissorMode`, `GetMousePosition`, `GetMouseWheelMove`, `IsKeyPressed`, `IsMouseButtonPressed/Released` |
| `ReaderScreen.cpp` | `GetScreenWidth/H`, `GetMousePosition`, `IsKeyPressed`, `IsMouseButtonPressed`, `IsMouseButtonDown` |
| `SettingsScreen.cpp` | `GetScreenWidth`, `GetMousePosition`, `IsKeyPressed`, `IsMouseButtonPressed/Released` |
| `components.cpp` | `GetMousePosition`, `IsMouseButtonDown`, `IsMouseButtonReleased` |

**Key problem functions:**
- `BeginScissorMode` / `EndScissorMode` — direct GPU state management by 3 screens
- `GetScreenWidth` / `GetScreenHeight` — screen dimensions polled directly (7 screens)
- `GetMousePosition` — input state polled directly (8 files)
- `IsKeyPressed` / `IsMouseButtonPressed` — input state polled directly (7 files)

**Current state:** Every screen performs its own input polling and rendering setup.
The `Screen` base class (`src/ui/Screen.h`) has only 2 methods: `Draw()` and
`HandleInput()`. No context, no injection of screen dimensions or input state.

**Intended state:** Screens should receive necessary context (viewport dimensions,
input state, fonts) through constructor injection or a `DrawContext` parameter.
No screen should call `GetScreenWidth`, `GetMousePosition`, `BeginScissorMode` directly.

**Why it's a problem:**
1. Renderer is supposed to be the single rendering authority — screens circumvent it
2. Every screen duplicates the input-polling pattern (7 nearly identical loops)
3. Platform coupling leaks into every screen — impossible to write screens without raylib
4. Prevents alternative rendering backends (e.g., an off-screen renderer for testing)

**Remediation (medium-term, large effort):**
1. Define `DrawContext` struct: `{ screenWidth, screenHeight, mousePos, dpiScale, fonts... }`
2. Inject into `Screen::Draw(DrawContext&)` and `Screen::HandleInput(DrawContext&)` — change
   the virtual signatures
3. Update all 9 screen/component files to use `DrawContext` instead of direct calls
4. Add clipping abstraction (e.g., `context.PushClipRect(x, y, w, h)` / `PopClipRect()`)
5. Remove `<raylib.h>` includes from screen files where possible

**This is a large refactor.** Due to the scope, it may be deferred to v1.9.0+.
A lighter alternative: extract input-state + viewport into a single `FrameState`
struct passed through the existing Draw()/HandleInput() signatures without changing
the Screen base class (wrap at App level).

**Effort:** Large (one dedicated session per screen ~3-4 hours total, or defer)
**Recommendation:** Document pattern, fix in dedicated refactor release.

---

### A5 — LayoutEngine Depends on Raylib Font

**Files:**
- `src/text/LayoutEngine.h:7` — `#include <raylib.h>`
- `src/text/LayoutEngine.h:22-25` — `const Font& bodyFont, headingFont_, largeFont_, smallFont_`
- `src/text/LayoutEngine.cpp:155,172,230,241,312,323,346` — 7 calls to `MeasureTextEx()`

**Current state:** The `LayoutEngine` stores 4 raylib `Font` references and uses
`MeasureTextEx()` to measure word widths. This is the only raylib dependency in the
text module.

**Intended state:** The text layout engine should be a pure data-transformation module.
Text measurement should be abstracted behind an interface.

**Why it's a problem:**
1. Pure computation depends on a graphics library
2. Cannot layout text without initializing raylib (affects testability)
3. Locked into raylib's `MeasureTextEx` font metrics forever

**Remediation:**
1. Define `struct TextMeasureInput { std::string text; float fontSize; float spacing; }`
   and `struct TextMeasureOutput { float width; float height; }` in `core/` or `text/`.
2. Define an interface (or function pointer type):
   ```cpp
   using TextMeasureFn = std::function<TextMeasureOutput(const TextMeasureInput&)>;
   ```
3. Accept `TextMeasureFn measureFn` in LayoutEngine constructor.
4. Create raylib implementation at App level:
   ```cpp
   TextMeasureOutput RaylibMeasure(const TextMeasureInput& input) {
       auto sz = MeasureTextEx(fontMap[input.fontSize], ...);
       return {sz.x, sz.y};
   }
   ```
5. Remove `<raylib.h>` from LayoutEngine.h, replace `Font&` members with measure function
   (or a font-size-indexed measure cache).

**Note:** This requires LayoutEngine to also know which font scale maps to which font
resource — currently it receives 4 separate Font references. The measure abstraction
would need to handle font-face selection as well.

**Effort:** Medium (~1-2 hours)

---

### A6 — Highlighter Depends on Raylib Color

**Files:**
- `src/highlight/Highlighter.h:7` — `#include <raylib.h>`
- `src/highlight/Highlighter.h:24-25` — `Color GetHighlightForWord(...)` return type

**Current state:** `Highlighter::GetHighlightForWord()` returns raylib `Color`.
Internally, `Color` values are constructed via aggregate init from `SimpleColor`
(defined in `core/Theme.h`).

**Intended state:** Highlighter should return a portable color value, not a raylib type.

**Why it's a problem:**
1. Cross-cutting module depends on a rendering framework
2. Prevents use of Highlighter in non-raylib contexts

**Remediation:**
1. Define `Rgba` struct in `core/Theme.h` (or migrate from `SimpleColor`):
   ```cpp
   struct Rgba { unsigned char r, g, b, a; };
   ```
2. Change `GetHighlightForWord()` to return `Rgba`.
3. Convert at the renderer boundary: `Color{ rgba.r, rgba.g, rgba.b, rgba.a }`.
4. Remove `#include <raylib.h>` from `Highlighter.h`.

**Files to change:**
- `src/core/Theme.h` — ensure `Rgba` or extend `SimpleColor` with alpha
- `src/highlight/Highlighter.h` — change return type, remove raylib include
- `src/highlight/Highlighter.cpp` — change local `ToColor()` to return Rgba
- `src/renderer/Renderer.cpp` — convert Rgba to Color at draw time
- `src/renderer/UIManager.cpp` — same conversion
- `src/renderer/RadialMenu.cpp` — same conversion

**Effort:** Small (~30 min)

---

### A7 — `#ifdef _WIN32` in PersistenceManager

**Files:**
- `src/persistence/PersistenceManager.cpp:35-39` — `#ifdef _WIN32` / `_mkdir()`

**Current state:** `EnsureDirectory()` uses `#ifdef _WIN32` directly to choose between
`_mkdir()` and `mkdir()`. This is the only platform-leaked `#ifdef` outside `core/`.

**Intended state:** Directory creation should be abstracted by `core/Platform.h`.

**Why it's a problem:**
1. Platform abstraction pattern is violated
2. If a new platform is added, every such `#ifdef` must be found and updated

**Remediation:**
1. Add `void Platform::EnsureDirectoryExists(const std::string& path)` to
   `src/core/Platform.h/cpp`.
2. Move the `#ifdef` block there, keeping platform logic centralized.
3. Call `Platform::EnsureDirectoryExists()` from `PersistenceManager.cpp` instead.
4. Remove `#include <sys/stat.h>` and `#ifdef _WIN32` from `PersistenceManager.cpp`.

**Files to change:**
- `src/core/Platform.h` — add declaration
- `src/core/Platform.cpp` — add implementation
- `src/persistence/PersistenceManager.cpp` — replace inline code with call

**Effort:** Trivial (~15 min)

---

### A8 — Event Bus Usage Is Ad-Hoc

**Current state:** 5 modules emit events, 6 modules subscribe, with no central registry
or documented contracts. The full event matrix:

| Event | Emitter | Subscribers | Notes |
|-------|---------|-------------|-------|
| `ScrollEvent` | InputHandler | ReaderScreen, ChapterGridScr, HighlightBrowserScr, BookListScr, DocumentManager | 1 emitter → 5 subs |
| `SelectionEvent` | App | Highlighter | 1→1 |
| `ResizeEvent` | InputHandler | App, LayoutEngine, DocumentManager | 1→3 |
| `FontSizeEvent` | InputHandler, SettingsScreen | App, DocumentManager | 2→2 |
| `SourceSwitchEvent` | SettingsScreen | App, DocumentManager | 1→2 |
| `DialogEvent` | InputHandler | InputHandler (self) | Self-subscribe |
| `KeyEvent` | InputHandler | *(none)* | Dead code |
| `NavigateEvent` | ReaderScreen, ChapterGridScr | App | 2→1 |
| `NavigateToHighlightEvent` | HighlightBrowserScr | ReaderScreen | 1→1 |
| `ChapterLoadedEvent` | DocumentManager | App | 1→1 |

**Issues:**
1. `KeyEvent` is emitted but never subscribed to — dead code
2. No unsubscribe mechanism — subscribers live for EventBus lifetime
3. No documentation of event contracts (expected payload shape, timing guarantees)
4. `LayoutEngine` emits nothing but subscribes to `ResizeEvent` — could use direct injection
5. `DialogEvent` self-subscribe in InputHandler is a code smell

**Remediation:**
1. Remove `KeyEvent` emission (dead code).
2. Add an events table to `Architecture Overview.md` documenting emitter/subscribers.
3. Consider replacing `DialogEvent` self-subscribe with direct method call.
4. No code change needed for the subscription pattern itself — it works.

**Files to change:**
- `src/input/InputHandler.cpp` — remove KeyEvent emission (line 89)
- `Architecture Overview.md` — add event governance section

**Effort:** Small (~30 min)

---

### A9 — App.cpp Is a 757-Line God Orchestrator

**Files:**
- `src/app/App.h` — 85 lines
- `src/app/App.cpp` — 757 lines, 22 includes, 7 `using namespace` declarations

**Current state:** `App` handles:
- Window creation and fonts (lines 1-51 in Init)
- Subsystem creation and wiring (constructor)
- Event subscription wiring (all .On<>() calls)
- Render loop orchestration
- Selection flow (from input to highlighter)
- Version switching
- Navigation event handling
- Chapter loading flow

**Intended state:** `App` should be a thin orchestrator — it creates subsystems and
delegates. Most event wiring and flow logic should live in the respective modules or
in a dedicated wiring layer.

**Why it's a problem:**
1. Single file knows about every module — high cognitive load
2. Any change in any module may require changing App.cpp
3. The selection logic (lines ~690-720) is raw re-emission of
   InputHandler callbacks — should be in Highlighter directly
4. Font size event handler (lines 337-358) manually calls 3 modules —
   should be delegated

**Remediation (progressive):**
1. Extract selection flow: `Highlighter` should directly subscribe to
   InputHandler's callbacks via EventBus, not go through App.
   (Currently App subscribes to InputHandler selection callbacks and
   re-emits SelectionEvent for Highlighter — an unnecessary hop.)
2. Extract font size change into a `FontManager` class that owns font
   resources and handles size changes.
3. Move ResizeEvent handler logic into DocumentManager + LayoutEngine
   (they already subscribe directly, App duplicates the handling).

**Note:** Items 2-3 are medium-to-large refactors. Item 1 is small and
immediately beneficial.

**Effort:** Large (each extraction is 1-2 sessions)
**Recommendation:** Start with selection flow extraction (Item 1).

---

## Remediation Roadmap

### Immediate (v1.8.0-alpha) ✅ Complete

| Finding | Action | Effort |
|---------|--------|--------|
| A1 | Update architecture docs to include `ui/` module | Trivial |
| A3 | Move NavigationStack::HandleInput from InputHandler to App (Option B) | Small |
| A6 | Replace raylib Color in Highlighter with portable SimpleColor | Small |
| A7 | Move `#ifdef _WIN32` to Platform::EnsureDirectoryExists | Trivial |
| A8 | Remove dead KeyEvent emission + add event governance to docs | Small |

### Short-Term (v1.9.0-alpha — Refactor Release)

| Finding | Action | Effort |
|---------|--------|--------|
| A2 | Extract Span/Line/ChapterLayout from ChapterProvider.h to text/LayoutTypes.h | Medium |
| A5 | Abstract MeasureTextEx behind TextMeasureFn interface | Medium |
| A9.1 | Remove selection re-emission hop through App | Small |

### Long-Term (Deferred)

| Finding | Action | Effort |
|---------|--------|--------|
| A4 | Introduce DrawContext, eliminate direct raylib calls from all screens | Large |
| A9.2-3 | Extract FontManager, move resize handling from App | Medium-Large |

---

## Updated Architecture Diagram (After Remediation)

```
┌─────────────────────────────────────────────────────┐
│                     APP LAYER                        │
│  App.cpp (thinner orchestrator)                      │
│  ├── wires subsystems                                │
│  ├── delegates selection flow to EventBus            │
│  └── delegates input routing to NavigationStack      │
├─────────────────────────────────────────────────────┤
│                 UI LAYER                              │
│  ├── renderer/   Renderer · UIManager · RadialMenu   │
│  └── ui/         Screen-based UI (NavigationStack)   │
│                  ├── Screen (base class)              │
│                  │     + Draw(DrawContext&)           │
│                  │     + HandleInput(DrawContext&)    │
│                  ├── ReaderScreen, BookListScreen,    │
│                  │   ChapterGridScreen, SettingsScr,  │
│                  │   HighlightBrowserScr, CenterMenu, │
│                  │   CreditsOverlay, FontDiagnostic   │
│                  ├── TapDetector                      │
│                  └── components.h                     │
├─────────────────────────────────────────────────────┤
│                 INPUT LAYER                           │
│  InputHandler (no UI dependency)                      │
│  ├── raw keyboard/mouse/touch → EventBus events      │
│  └── no NavigationStack knowledge                    │
├─────────────────────────────────────────────────────┤
│             DOCUMENT MANAGER                          │
│  DocumentManager  (holds ChapterProvider&)            │
├─────────────────────────────────────────────────────┤
│             TEXT LAYOUT ENGINE                        │
│  LayoutEngine (receives TextMeasureFn)                │
│  └── LayoutTypes.h (Span, Line, ChapterLayout)        │
├─────────────────────────────────────────────────────┤
│               DATA LAYER                              │
│  ChapterProvider (interface)                          │
│    ├── USFMParser   (offline)                         │
│    ├── BibleClient  (online)                          │
│    └── CompositeProvider (fallback chain)             │
│  PersistenceManager (SQLite)                          │
├─────────────────────────────────────────────────────┤
│             CROSS-CUTTING                             │
│  core/  → Platform, Config, Theme, EnvLoader          │
│  event/ → EventBus (pub-sub, no business logic)       │
│  highlight/ → Highlighter (portable Rgba)             │
│  persistence/ → PersistenceManager (platform-free)    │
└─────────────────────────────────────────────────────┘

Dependency arrow: only downward (core ← data ← text ← document ← input/ui ← app)
Cross edges: highlight → data/ChapterProvider (via Highlighter.h — accept for now,
             or extract to event-only communication)
```

---

## Appendix: File Inventory

### `src/ui/` — All files and their roles

| File | Role | Lines | Depends on |
|------|------|-------|------------|
| `Screen.h` | Abstract base class | 19 | (none) |
| `NavigationStack.h/cpp` | Screen stack management | ~80 | Screen |
| `ReaderScreen.h/cpp` | Main reading view | 73+340 | Screen, DocMgr, Highlighter, Renderer |
| `BookListScreen.h/cpp` | Book selection list | ~60+303 | Screen, UIScale, Locale, TapDetector |
| `ChapterGridScreen.h/cpp` | Chapter grid picker | ~55+279 | Screen, UIScale, TapDetector |
| `SettingsScreen.h/cpp` | Settings panel | ~40+255 | Screen, UIScale, Locale, TapDetector |
| `HighlightBrowserScreen.h/cpp` | Highlight browsing | ~60+345 | Screen, UIScale, Locale, TapDetector |
| `CenterMenu.h/cpp` | Navigation menu overlay | ~35+150 | Screen, UIScale, Locale, TapDetector |
| `CreditsOverlay.h/cpp` | About/credits popup | ~30+130 | Screen, UIScale, Locale, TapDetector |
| `FontDiagnostic.h/cpp` | Debug font inspector | ~25+185 | Screen, Locale |
| `TapDetector.h` | Touch/click helper | ~35 | (none) |
| `components.h/cpp` | Shared UI widgets | ~25+160 | UIScale |

### Cross-module include dependencies (non-core headers)

```
data/ChapterProvider.h:11 includes
  ├── data/ itself (BibleClient, CompositeProvider, USFMParser, etc.)
  ├── ui/ReaderScreen.cpp       ← needs Span, SegmentType for rendering
  ├── highlight/Highlighter.h   ← needs Word, Segment for highlight context
  ├── text/LayoutEngine.h       ← needs ChapterData, Segment for layout
  ├── renderer/Renderer.h       ← needs Span for draw (A2 violation)
  └── document/DocumentManager.h ← needs everything

event/EventBus.h:6 includes
  ├── text/LayoutEngine.cpp     ← subscribes ResizeEvent
  ├── document/DocumentManager.cpp ← subscribes ScrollEvent, ResizeEvent, etc.
  ├── highlight/Highlighter.cpp ← subscribes SelectionEvent
  ├── input/InputHandler.cpp    ← emits ScrollEvent, subscribes DialogEvent
  ├── ui/*Screen*.cpp           ← all screens subscribe ScrollEvent
  └── app/App.cpp               ← subscribes NavigateEvent, etc.

core/Theme.h:9 includes (across all renderer/ and ui/ files)
  ├── renderer/Renderer.cpp
  ├── renderer/UIManager.cpp
  ├── renderer/RadialMenu.cpp
  ├── highlight/Highlighter.h (via #include "data/ChapterProvider.h")
  └── app/App.cpp
```
