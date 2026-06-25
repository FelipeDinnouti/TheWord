# TheWord — Current Refactoring Plan

> Status: In Progress | Started: 2026-06-24 | Target: 2026-Q3

## Strategic Goals

1. **Code quality**: Fix method naming, eliminate duplication, split monoliths, apply modern C++ idioms
2. **Multi-platform build**: CMake presets, proper Android ARM support, fix Windows/macOS gaps
3. **Architecture**: Moderate cleanup now, full re-architecture proposal documented for later

## Priority Order

```
Phase 0 (Naming) → Phase 1 (Quality) → Phase 2 (Build) → Phase 3 (Platform) → Phase 4 (Docs)
```

Phase 0 MUST be completed first — it touches every file and would conflict with structural changes.

---

## Phase 0 — Method Naming Convention Fix

**Goal:** Rename all methods from `camelCase` to `PascalCase` per convention.
**Approach:** Per-file mechanical rename. Verify build + tests after each file group.

### 0.1 LayoutEngine (`src/text/LayoutEngine.h/.cpp`)

- [ ] `layoutChapter` → `LayoutChapter`
- [ ] `getFontSize` → `GetFontSize`
- [ ] `setFontSize` → `SetFontSize`
- [ ] `getMaxWidth` → `GetMaxWidth`
- [ ] `setMaxWidth` → `SetMaxWidth`
- [ ] `invalidateCache` → `InvalidateCache`
- [ ] `hitTestLine` → `HitTestLine`
- [ ] `layoutWords` (private) → `LayoutWords`
- [ ] `layoutHeading` (private) → `LayoutHeading`

**Verify:** Build + tests pass.

### 0.2 DocumentManager (`src/document/DocumentManager.h/.cpp`)

- [ ] `loadInitialChapter` → `LoadInitialChapter`
- [ ] `update` → `Update`
- [ ] `scrollBy` → `ScrollBy`
- [ ] `scrollTo` → `ScrollTo`
- [ ] `getScrollY` → `GetScrollY`
- [ ] `getTotalHeight` → `GetTotalHeight`
- [ ] `getViewportHeight` → `GetViewportHeight`
- [ ] `setViewportHeight` → `SetViewportHeight`
- [ ] `invalidateLayouts` → `InvalidateLayouts`
- [ ] `getVisibleSpans` → `GetVisibleSpans`
- [ ] `hitTestWord` → `HitTestWord`
- [ ] `getCurrentChapterId` → `GetCurrentChapterId`
- [ ] `getChapterTitle` → `GetChapterTitle`
- [ ] `recalculateChapterPositions` (private) → `RecalculateChapterPositions`
- [ ] `tryLoadAdjacent` (private) → `TryLoadAdjacent`
- [ ] `prependChapter` (private) → `PrependChapter`
- [ ] `appendChapter` (private) → `AppendChapter`

**Verify:** Build + tests pass.

### 0.3 Renderer (`src/renderer/Renderer.h/.cpp`)

- [ ] `drawFrame` → `DrawFrame`
- [ ] `drawScrollbar` → `DrawScrollbar`
- [ ] `setFontSize` → `SetFontSize`
- [ ] `getFontSize` → `GetFontSize`
- [ ] `drawFpsCounter` → `DrawFpsCounter`
- [ ] `getContentTop` → `GetContentTop`
- [ ] `drawSpan` (private) → `DrawSpan`

**Verify:** Build + tests pass.

### 0.4 UIManager (`src/renderer/UIManager.h/.cpp`)

- [ ] `getContentTop` → `GetContentTop`
- [ ] `getFontSize` → `GetFontSize`
- [ ] `drawTopBar` → `DrawTopBar`
- [ ] `drawContextMenu` → `DrawContextMenu`
- [ ] `drawSettingsPanel` → `DrawSettingsPanel`
- [ ] `drawGoToDialog` → `DrawGoToDialog`
- [ ] `showContextMenu` → `ShowContextMenu`
- [ ] `hideContextMenu` → `HideContextMenu`
- [ ] `isContextMenuActive` → `IsContextMenuActive`
- [ ] `handleContextMenuClick` → `HandleContextMenuClick`
- [ ] `toggleGoToDialog` → `ToggleGoToDialog`
- [ ] `dismissGoToDialog` → `DismissGoToDialog`
- [ ] `isGoToDialogActive` → `IsGoToDialogActive`
- [ ] `handleGoToKeyboardInput` → `HandleGoToKeyboardInput`
- [ ] `handleGoToClick` → `HandleGoToClick`
- [ ] `toggleAbout` → `ToggleAbout`
- [ ] `dismissAbout` → `DismissAbout`
- [ ] `isAboutActive` → `IsAboutActive`
- [ ] `drawAbout` → `DrawAbout`
- [ ] `handleAboutClick` → `HandleAboutClick`
- [ ] `toggleSettings` → `ToggleSettings`
- [ ] `dismissSettings` → `DismissSettings`
- [ ] `isSettingsActive` → `IsSettingsActive`
- [ ] `handleSettingsClick` → `HandleSettingsClick`
- [ ] `changeFontSize` → `ChangeFontSize`
- [ ] `dismissActiveDialog` → `DismissActiveDialog`
- [ ] ~15 private helper methods (same pattern)

**Verify:** Build + tests pass.

### 0.5 InputHandler (`src/input/InputHandler.h/.cpp`)

- [ ] `handleInput` → `HandleInput`
- [ ] `handleScroll` → `HandleScroll`
- [ ] `handlePressFSM` → `HandlePressFSM`
- [ ] `handleWindowResize` → `HandleWindowResize`
- [ ] `handleRightClick` → `HandleRightClick`
- [ ] `handleTouchScroll` → `HandleTouchScroll`
- [ ] `handleTouchPressFSM` → `HandleTouchPressFSM`
- [ ] `handlePinch` → `HandlePinch`

**Verify:** Build + tests pass.

### 0.6 Highlighter (`src/highlight/Highlighter.h/.cpp`)

- [ ] `startSelection` → `StartSelection`
- [ ] `updateSelection` → `UpdateSelection`
- [ ] `endSelection` → `EndSelection`
- [ ] `isWordHighlighted` → `IsWordHighlighted`
- [ ] `getHighlightForWord` → `GetHighlightForWord`
- [ ] `highlightAtWord` → `HighlightAtWord`
- [ ] `removeHighlight` → `RemoveHighlight`
- [ ] `recolorHighlight` → `RecolorHighlight`
- [ ] `setActiveTypeId` → `SetActiveTypeId`
- [ ] `getActiveTypeId` → `GetActiveTypeId`
- [ ] `setProvider` → `SetProvider`
- [ ] `getProvider` → `GetProvider`
- [ ] `load` → `Load`
- [ ] `getHighlights` → `GetHighlights`
- [ ] `getTypes` → `GetTypes`

**Verify:** Build + tests pass.

### 0.7 InMemoryStorage + PersistenceManager (`src/highlight/`, `src/persistence/`)

- [ ] `loadHighlights` → `LoadHighlights`
- [ ] `saveHighlight` → `SaveHighlight`
- [ ] `removeHighlight` → `RemoveHighlight`
- [ ] `loadHighlightTypes` → `LoadHighlightTypes`
- [ ] `saveHighlightType` → `SaveHighlightType`
- [ ] `getPreference` → `GetPreference`
- [ ] `setPreference` → `SetPreference`
- [ ] `initSchema` (private) → `InitSchema`
- [ ] `ensureDirectory` (private) → `EnsureDirectory`

**Verify:** Build + tests pass.

### 0.8 Data Layer (`src/data/`)

- [ ] `BibleClient`: `extractJsonString` → `ExtractJsonString`, `parseHtmlChapter` → `ParseHtmlChapter`, `stripHtml` → `StripHtml`
- [ ] `CompositeProvider`: `setPrimary` → `SetPrimary`
- [ ] `USFMParser`: `parseBook` → `ParseBook`, `loadFile` → `LoadFile`, `stripFootnotes` → `StripFootnotes`, `stripInlineMarkers` → `StripInlineMarkers`, `extractBookCodeFromId` → `ExtractBookCodeFromId`
- [ ] `StubChapterProvider`: file-static `tokenizeText` → `TokenizeText`

**Verify:** Build + tests pass.

### 0.9 File-Static Functions (`src/core/`)

- [ ] `EmscriptenClient.cpp`: `onSuccess` → `OnSuccess`, `onError` → `OnError`
- [ ] `FontHelper.cpp`: `ReadU16` → `ReadU16` (already PascalCase), `ReadU32` → `ReadU32` (already PascalCase — no change needed)
- [ ] `Highlighter.cpp`: `DEFAULT_HIGHLIGHT_TYPE` (variable, already SNAKE_CASE), `ToColor` → `ToColor` (already PascalCase — no change needed)

**Verify:** Build passes.

### 0.10 Test Helpers (`tests/`)

- [ ] `MockHttpClient`: `setResponse` → `SetResponse`, `get` → `Get`, `setAppKey` → `SetAppKey`, `getAppKey` → `GetAppKey`

**Verify:** Build + tests pass.

### 0.11 Update Call Sites (`src/main.cpp`, `tests/test_main.cpp`, `src/renderer/UIManager.cpp`, `src/input/InputHandler.cpp`)

- [ ] Update all references in `main.cpp`
- [ ] Update all references in `test_main.cpp`
- [ ] Update all cross-module call sites in `UIManager.cpp`
- [ ] Update all cross-module call sites in `InputHandler.cpp`
- [ ] Update all cross-module call sites in `Renderer.cpp`

**Final Verify:** Build + all 64 tests pass.

---

## Phase 1 — Code Quality Refactoring

### 1.1 Eliminate Copy-Paste Duplication

- [ ] `AndroidClient.h/.cpp`: Remove entirely. Use `CurlHttpClient` for both desktop and Android.
- [ ] `CMakeLists.txt`: Remove AndroidClient sources, wire CurlHttpClient for Android.
- [ ] `EnvLoader.cpp`: Extract shared line-parsing into `ParseLine` helper. Eliminate 95% duplication between `load()` and `loadFromContent()`.

### 1.2 Split Monolithic Functions

- [ ] `BibleClient.cpp`: Extract from `parseHtmlChapter` (~290 lines):
  - `ParseParagraphContent` — handles `<p>` divs with verse navigation
  - `ParsePoetryLine` — handles `<q1>`, `<q2>` divs
  - `ParseSectionHeading` — handles `<s1>`, `<s2>` divs
  - `StripFootnote` — handles `<f>` tags
- [ ] `USFMParser.cpp`: Extract from `parseBook` (~264 lines):
  - Dispatch table mapping USFM markers to handler methods
  - `HandleVerseMarker`, `HandleSectionHeading(s#)`, `HandlePoetryLine(q#)`, `HandleParagraphBreak(p/m)`, `HandleBookTitle(mt#)`, `HandleChapterLabel(c)`, `HandleDescription(d)`, `HandleRemark(r)`

### 1.3 Split UIManager (591 lines → focused components)

- [ ] Create `renderer/ContextMenu.h/.cpp` — context menu rendering + click handling
- [ ] Create `renderer/SettingsPanel.h/.cpp` — settings panel rendering + interaction
- [ ] Create `renderer/GoToDialog.h/.cpp` — go-to navigation dialog
- [ ] Create `renderer/AboutOverlay.h/.cpp` — about/credits overlay
- [ ] `UIManager` becomes thin coordinator owning instances of above components

### 1.4 Fix Bugs

- [ ] `BibleClient::HasChapter()`: Implement real chapter existence check
- [ ] `BibleClient::parseHtmlChapter`: Fix `inFootnote` flag never reset in paragraph section
- [ ] `PersistenceManager`: Guard all `sqlite3_finalize` calls; propagate SQLite errors upward
- [ ] Replace all `std::atoi` with `std::from_chars` or `std::stoi` with validation

### 1.5 Modern C++ Cleanup

- [ ] `CurlHttpClient`: `void* curl` → `std::unique_ptr<CURL, decltype(&curl_easy_cleanup)>`
- [ ] Remove unused includes (<iostream>, redundant <cmath>, <cstdio>)
- [ ] Fix include paths: `../data/ChapterProvider.h` → `data/ChapterProvider.h`
- [ ] Fix include ordering (std → raylib → project headers)
- [ ] Remove redundant forward declarations
- [ ] Wrap file-scope `static` in anonymous namespaces
- [ ] Add `const` to `PersistenceManager::getPreference`, `BibleClient::parseHtmlChapter`
- [ ] Remove decorative comments (section dividers, obvious annotations)
- [ ] Remove `friend class BibleClientTest`; test through public API or protected accessor

### 1.6 Verify

- [ ] Build succeeds with no warnings
- [ ] All 64 tests pass
- [ ] App runs and renders correctly

---

## Phase 2 — Build System Modernization

### 2.1 CMake Presets

- [ ] Create `CMakePresets.json` with presets:
  - `default` — Linux Desktop Release
  - `debug` — Linux Desktop Debug
  - `android-x86_64` — Android x86_64 (emulator)
  - `android-arm64` — Android arm64-v8a (device)
  - `wasm` — WebAssembly (Emscripten)
  - `windows-mingw` — Windows cross-compile

### 2.2 Modern CMake Practices

- [ ] Replace global `CMAKE_CXX_STANDARD` with `target_compile_features(theword PUBLIC cxx_std_17)`
- [ ] Replace `${CURL_LIBRARIES}` with `CURL::libcurl` imported target
- [ ] Create `theword_objects` OBJECT library to eliminate duplicate source lists
- [ ] Add `-Wall -Wextra -Wpedantic` with MSVC guard
- [ ] Add `set(CMAKE_EXPORT_COMPILE_COMMANDS ON)` for clangd support
- [ ] Fix asset copying: `file(COPY ...)` → `add_custom_command(TARGET ... POST_BUILD)`
- [ ] Add `SYSTEM` qualifier for sqlite3 include directory
- [ ] List header files in source lists for IDE visibility

### 2.3 Platform Fixes

- [ ] **Windows**: Add `find_package(CURL)` for WIN32 branch
- [ ] **macOS**: Add `APPLE` branch with `CURL::libcurl` + Cocoa/IOKit frameworks
- [ ] **Android**: Remove redundant `PLATFORM` cache variable

### 2.4 Verify

- [ ] `cmake --preset default` configures and builds
- [ ] `cmake --preset debug` configures and builds
- [ ] `cmake --build build && ./build/theword` runs correctly
- [ ] Clean reconfigure works: `rm -rf build && cmake --preset default`

---

## Phase 3 — Android ARM + Cross-Platform Polish

### 3.1 Android ARM Support

- [ ] Update `scripts/build-android.sh` to build for `x86_64`, `arm64-v8a`, `armeabi-v7a`
- [ ] Create `src/main/java/com/theword/app/TheWordActivity.java` (Java activity stub)
- [ ] Fix Android DB path: use `android_app->activity->internalDataPath` instead of hardcoded path
- [ ] Verify `GetAndroidApp()` resolves correctly from raylib's Android backend

### 3.2 WASM Persistence

- [ ] Add `-lidbfs.js` to Emscripten link flags in `CMakePresets.json`
- [ ] Mount IDBFS in `wasm_shell.html`
- [ ] Add `__EMSCRIPTEN__` DB path branch in `main.cpp`: `/persistent/theword.db`

### 3.3 Build Scripts

- [ ] Create `scripts/build-wasm.sh` for Emscripten builds
- [ ] Update `README.md` and `Build Guide.md` with preset-based workflow

### 3.4 libcurl Optional

- [ ] `find_package(CURL QUIET)` instead of `REQUIRED`
- [ ] Gate `BibleClient` and composite provider behind `CURL_FOUND`
- [ ] Document optional dependency in README

### 3.5 Asset Loading Abstraction

- [ ] Refactor `FontHelper` to accept `IAssetProvider*` or raw font data buffer
- [ ] Centralize font loading through asset provider in `main.cpp`

### 3.6 Platform Dispatch

- [ ] Create `src/core/Platform.h` with platform info struct (DB path, asset root, DPI scale)
- [ ] Consolidate scattered `#ifdef` blocks from `main.cpp` into Platform module

### 3.7 Verify

- [ ] Android APK builds for both x86_64 and arm64-v8a
- [ ] WASM builds and persists highlights across page reload
- [ ] Linux build works without libcurl installed (USFM-only mode)

---

## Phase 4 — Documentation

### 4.1 Full Re-Architecture Proposal

- [ ] Write `the-word-docs/07-ai-collaboration/Full Re-architecture Proposal.md` covering:
  - Namespace strategy for all modules
  - Proper module boundaries with clear ownership
  - Full platform abstraction layer design
  - Event system for decoupling input → document → renderer
  - Ideal state splitting: what UIManager, DocumentManager, InputHandler should look like

### 4.2 Convention Updates

- [ ] Update `the-word-docs/07-ai-collaboration/Convention Reference.md` with any changes

### 4.3 Progress Tracking

- [ ] Update `the-word-docs/04-planning/Progress Tracking.md` with completed items

### 4.4 Build Guide

- [ ] Update `the-word-docs/06-ops/Build Guide.md` with new CMake preset workflow
- [ ] Update `README.md` with simplified build instructions

---

## Acceptance Criteria

| Phase | Criterion |
|-------|-----------|
| 0 | `cmake --build build` succeeds, `./build/theword_test` passes all 64 tests, app runs |
| 1 | Same as above + no compiler warnings, no unused includes, no `void*` ownership |
| 2 | `cmake --preset <name>` works for all target platforms, no deprecation warnings |
| 3 | Android APK installs and runs on emulator + ARM device; WASM persists state |
| 4 | All docs reviewed and consistent with current codebase state |

## Current Status

- **Phase 0**: ✅ Complete (2026-06-24) — All ~80+ methods renamed from camelCase to PascalCase. Build clean, 64/64 tests pass.
- **Phase 1–4**: Not started
