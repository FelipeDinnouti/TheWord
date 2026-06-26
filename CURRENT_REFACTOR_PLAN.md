# TheWord — Current Refactoring Plan

> Status: Active | Updated: 2026-06-26

---

## Priority Order

```
Phase 0 (Naming) → Phase 1 (Docs) → Phase 2 (Quality) → Phase 3 (Build) → Phase 4 (Platform) → Phase 5 (Re-Architecture)
```

Phase 0 is already complete. Phase 1 (Docs) was completed 2026-06-26.

---

## Phase 0 — Method Naming Convention Fix ✅ Complete (2026-06-24)

**Goal:** Rename all methods from `camelCase` to `PascalCase` per convention.
**Status:** ✅ All ~80+ methods renamed. Build clean, 64/64 tests pass.

### 0.1 Verified Files
- `src/text/LayoutEngine.h/.cpp` — 9 methods
- `src/document/DocumentManager.h/.cpp` — 18 methods
- `src/renderer/Renderer.h/.cpp` — 7 methods
- `src/renderer/UIManager.h/.cpp` — ~25 methods
- `src/input/InputHandler.h/.cpp` — 8 methods
- `src/highlight/Highlighter.h/.cpp` — 15 methods
- `src/highlight/InMemoryStorage.h/.cpp` + `src/persistence/PersistenceManager.h/.cpp` — 9 methods
- `src/data/` — BibleClient, CompositeProvider, USFMParser methods
- `src/core/` — file-static functions
- `tests/` — MockHttpClient methods
- All call sites updated in `main.cpp`, `test_main.cpp`, `UIManager.cpp`, `InputHandler.cpp`, `Renderer.cpp`

---

## Phase 1 — Doc Freshness Audit & Build Flow Documentation ✅ Complete (2026-06-26)

**Goal:** Every doc matches the actual codebase. Build flow (artifacts, output locations, platform commands) is fully documented. New contributors can go from zero to running on any platform using only `the-word-docs/`.

**Why this matters:** The code had sprinted far ahead of documentation. Outdated docs mislead contributors and create compounding tech debt. This phase touched no source code, just `the-word-docs/` and `README.md`.

### 1.1 Edited Files

- `06-ops/Build Guide.md` — Complete rewrite: artifact locations, all 4 platforms, test commands
- `06-ops/Environment Setup.md` — Added Android NDK + Emscripten setup
- `06-ops/Troubleshooting.md` — Added Android, WASM, Windows entries
- `04-planning/Progress Tracking.md` — Phase 10 status corrected to In Progress
- `04-planning/Development Plan.md` — Phase 9 status corrected to Complete
- `04-planning/Roadmap.md` — Updated to reflect completed phases
- `01-vision/Product Requirements.md` — Checked off all implemented features
- `03-modules/Persistence.md` — Added `provider_name` column to schema
- `03-modules/UI Layer.md` — Added Sprint 4 polish content
- `03-modules/Highlighting System.md` — Status changed to Complete
- `02-architecture/Architecture Overview.md` — Android build section updated
- `02-architecture/Data Flow.md` — Added Android render loop variant
- `README.md` — Updated feature list and controls

---

## Phase 2 — Code Quality Refactoring

**Goal:** Eliminate duplication, split monoliths, fix bugs, apply modern C++.

### 2.1 Eliminate Copy-Paste Duplication

- [ ] `AndroidClient.h/.cpp`: Remove entirely. Use `CurlHttpClient` for both desktop and Android.
- [ ] `CMakeLists.txt`: Remove AndroidClient sources, wire CurlHttpClient for Android.
- [ ] `EnvLoader.cpp`: Extract shared line-parsing into `ParseLine` helper. Eliminate 95% duplication between `load()` and `loadFromContent()`.

### 2.2 Split Monolithic Functions

- [ ] `BibleClient.cpp`: Extract from `parseHtmlChapter` (~290 lines):
  - `ParseParagraphContent` — handles `<p>` divs with verse navigation
  - `ParsePoetryLine` — handles `<q1>`, `<q2>` divs
  - `ParseSectionHeading` — handles `<s1>`, `<s2>` divs
  - `StripFootnote` — handles `<f>` tags
- [ ] `USFMParser.cpp`: Extract from `parseBook` (~264 lines):
  - Dispatch table mapping USFM markers to handler methods
  - `HandleVerseMarker`, `HandleSectionHeading(s#)`, `HandlePoetryLine(q#)`, `HandleParagraphBreak(p/m)`, `HandleBookTitle(mt#)`, `HandleChapterLabel(c)`, `HandleDescription(d)`, `HandleRemark(r)`

### 2.3 Split UIManager (591 lines → focused components)

- [ ] Create `renderer/ContextMenu.h/.cpp` — context menu rendering + click handling
- [ ] Create `renderer/SettingsPanel.h/.cpp` — settings panel rendering + interaction
- [ ] Create `renderer/GoToDialog.h/.cpp` — go-to navigation dialog
- [ ] Create `renderer/AboutOverlay.h/.cpp` — about/credits overlay
- [ ] `UIManager` becomes thin coordinator owning instances of above components

### 2.4 Fix Bugs

- [ ] `BibleClient::HasChapter()`: Implement real chapter existence check
- [ ] `BibleClient::parseHtmlChapter`: Fix `inFootnote` flag never reset in paragraph section
- [ ] `PersistenceManager`: Guard all `sqlite3_finalize` calls; propagate SQLite errors upward
- [ ] Replace all `std::atoi` with `std::from_chars` or `std::stoi` with validation

### 2.5 Modern C++ Cleanup

- [ ] `CurlHttpClient`: `void* curl` → `std::unique_ptr<CURL, decltype(&curl_easy_cleanup)>`
- [ ] Remove unused includes (`<iostream>`, redundant `<cmath>`, `<cstdio>`)
- [ ] Fix include paths: `../data/ChapterProvider.h` → `data/ChapterProvider.h`
- [ ] Fix include ordering (std → raylib → project headers)
- [ ] Remove redundant forward declarations
- [ ] Wrap file-scope `static` in anonymous namespaces
- [ ] Add `const` to `PersistenceManager::getPreference`, `BibleClient::parseHtmlChapter`
- [ ] Remove decorative comments (section dividers, obvious annotations)
- [ ] Remove `friend class BibleClientTest`; test through public API or protected accessor

### 2.6 Verify

- [ ] Build succeeds with no warnings
- [ ] All 64 tests pass
- [ ] App runs and renders correctly

---

## Phase 3 — Build System Modernization

### 3.1 CMake Presets

Create `CMakePresets.json` with presets:

| Preset | Platform | Generator | Notes |
|--------|----------|-----------|-------|
| `default` | Linux Desktop Release | Unix Makefiles | Standard build |
| `debug` | Linux Desktop Debug | Unix Makefiles | Debug flags |
| `android-x86_64` | Android x86_64 (emulator) | Ninja | NDK toolchain |
| `android-arm64` | Android arm64-v8a (device) | Ninja | NDK toolchain |
| `wasm` | WebAssembly (Emscripten) | Ninja | Emserver toolchain |
| `windows-mingw` | Windows cross-compile | MinGW Makefiles | From Linux host |

### 3.2 Modern CMake Practices

- [ ] Replace global `CMAKE_CXX_STANDARD` with `target_compile_features(theword PUBLIC cxx_std_17)`
- [ ] Replace `${CURL_LIBRARIES}` with `CURL::libcurl` imported target
- [ ] Create `theword_objects` OBJECT library to eliminate duplicate source lists
- [ ] Add `-Wall -Wextra -Wpedantic` with MSVC guard
- [ ] Add `set(CMAKE_EXPORT_COMPILE_COMMANDS ON)` for clangd support
- [ ] Fix asset copying: `file(COPY ...)` → `add_custom_command(TARGET ... POST_BUILD)`
- [ ] Add `SYSTEM` qualifier for sqlite3 include directory
- [ ] List header files in source lists for IDE visibility

### 3.3 Platform Fixes

- [ ] **Windows**: Add `find_package(CURL)` for WIN32 branch
- [ ] **macOS**: Add `APPLE` branch with `CURL::libcurl` + Cocoa/IOKit frameworks
- [ ] **Android**: Remove redundant `PLATFORM` cache variable

### 3.4 Verify

- [ ] `cmake --preset default` configures and builds
- [ ] `cmake --preset debug` configures and builds
- [ ] `cmake --build build && ./build/theword` runs correctly
- [ ] Clean reconfigure: `rm -rf build && cmake --preset default`

---

## Phase 4 — Android ARM + Cross-Platform Polish

### 4.1 Android ARM Support

- [ ] Update `scripts/build-android.sh` to build for `x86_64`, `arm64-v8a`, `armeabi-v7a` (multi-ABI or parameterized)
- [ ] Create `src/main/java/com/theword/app/TheWordActivity.java` (Java activity stub for splash/IME)
- [ ] Fix Android DB path: confirm `/data/data/com.theword.app/app_storage/` works on all devices
- [ ] Verify `GetAndroidApp()` resolves correctly from raylib's Android backend

### 4.2 WASM Persistence

- [ ] Add `-lidbfs.js` to Emscripten link flags in `CMakePresets.json`
- [ ] Mount IDBFS in `wasm_shell.html`
- [ ] Add `__EMSCRIPTEN__` DB path branch in `main.cpp`: `/persistent/theword.db`

### 4.3 Build Scripts

- [ ] Create `scripts/build-wasm.sh` for Emscripten builds
- [ ] Update `README.md` and `Build Guide.md` with preset-based workflow

### 4.4 libcurl Optional

- [ ] `find_package(CURL QUIET)` instead of `REQUIRED`
- [ ] Gate `BibleClient` and composite provider behind `CURL_FOUND`
- [ ] Document optional dependency in README and Environment Setup

### 4.5 Asset Loading Abstraction

- [ ] Refactor `FontHelper` to accept `IAssetProvider*` or raw font data buffer
- [ ] Centralize font loading through asset provider in `main.cpp`

### 4.6 Platform Dispatch

- [ ] Create `src/core/Platform.h` with platform info struct (DB path, asset root, DPI scale)
- [ ] Consolidate scattered `#ifdef` blocks from `main.cpp` into Platform module

### 4.7 Verify

- [ ] Android APK builds for both x86_64 and arm64-v8a
- [ ] WASM builds and persists highlights across page reload
- [ ] Linux build works without libcurl installed (USFM-only mode)
- [ ] `scripts/build-wasm.sh` works end-to-end

---

## Phase 5 — Full Re-Architecture Proposal (Documentation Only)

### 5.1 Re-Architecture Document

- [ ] Write `the-word-docs/07-ai-collaboration/Full Re-architecture Proposal.md` covering:
  - Namespace strategy for all modules
  - Proper module boundaries with clear ownership
  - Full platform abstraction layer design
  - Event system for decoupling input → document → renderer
  - Ideal state splitting: what UIManager, DocumentManager, InputHandler should look like

### 5.2 Convention Updates

- [ ] Update `the-word-docs/07-ai-collaboration/Convention Reference.md` with any changes from Phase 1-2

### 5.3 Progress Tracking

- [ ] Update `the-word-docs/04-planning/Progress Tracking.md` with completed items from Phases 2-4

### 5.4 Build Guide Finalization

- [ ] Final pass on `06-ops/Build Guide.md` with new CMake preset workflow
- [ ] Update `README.md` with simplified preset-based build instructions

---

## Acceptance Criteria

| Phase | Criterion |
|-------|-----------|
| 0 | `cmake --build build` succeeds, `./build/theword_test` passes all 64 tests, app runs. |
| 1 | Every doc in `the-word-docs/` accurately reflects the codebase. Build flow fully documented for all platforms. |
| 2 | No compiler warnings, no unused includes, no `void*` ownership, monoliths split. |
| 3 | `cmake --preset <name>` works for all target platforms, no deprecation warnings. |
| 4 | Android APK builds for both x86_64 and arm64-v8a; WASM persists state; libcurl truly optional. |
| 5 | Re-architecture proposal available for future planning. All docs consistent with codebase. |

---

## Current Status

| Phase | Status | Notes |
|-------|--------|-------|
| **Phase 0: Naming Convention** | ✅ Complete | 64/64 tests pass, build clean |
| **Phase 1: Docs Freshness** | ✅ Complete | Edited 2026-06-26 — all stale docs updated, build flow documented |
| **Phase 2: Code Quality** | 🔴 In Progress | Actively working |
| **Phase 3: Build System** | ⬜ Not started | |
| **Phase 4: Cross-Platform** | ⬜ Not started | |
| **Phase 5: Re-Architecture** | ⬜ Not started | |
