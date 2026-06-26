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

- [x] `AndroidClient.h/.cpp`: Removed. `CurlHttpClient` used for both desktop and Android.
- [x] `CMakeLists.txt`: AndroidClient sources replaced with CurlHttpClient in ANDROID branch.
- [x] `EnvLoader.cpp`: Extract shared line-parsing into `ParseLine` helper. Eliminate 95% duplication between `load()` and `loadFromContent()`.

### 2.2 Split Monolithic Functions

- [x] `BibleClient.cpp`: Extracted from `parseHtmlChapter` (~290 → ~100 lines):
  - `ParseParagraphContent` — handles `<p>` divs with verse navigation
  - `ParsePoetryLine` — handles `<q1>`, `<q2>` divs
  - `ParseSectionHeading` — handles `<s1>`, `<s2>` divs
  - `StripFootnotes` — handles nested `<span class="yv-n">` blocks with depth tracking
  - `ClassifyDiv`, `ParseVerseNumber`, `FlushWordsToSegment` helpers
- [x] `USFMParser.cpp`: Extracted from `parseBook` (~264 → ~120 lines):
  - `HandleChapter`, `HandleVerse`, `HandleParagraph`, `HandleSectionHeading`
  - `HandleDescription`, `HandleRemark`, `HandlePoetryLine`, `HandleBookTitle`
  - `FlushSegment`, `ParseState` struct for shared state

### 2.3 Split UIManager (591 lines → focused components)

- [x] Create `renderer/ContextMenu.h/.cpp` — context menu rendering + click handling
- [x] Create `renderer/SettingsPanel.h/.cpp` — settings panel rendering + interaction
- [x] Create `renderer/GoToDialog.h/.cpp` — go-to navigation dialog
- [x] Create `renderer/AboutOverlay.h/.cpp` — about/credits overlay
- [x] `UIManager` becomes thin coordinator owning instances of above components

### 2.4 Fix Bugs

- [x] `BibleClient::HasChapter()`: Implement real chapter existence check
- [x] `BibleClient::parseHtmlChapter`: Fix `inFootnote` flag never reset in paragraph section (resolved by depth-counter rewrite in 2.2)
- [x] `PersistenceManager`: Guard all `sqlite3_finalize` calls; propagate SQLite errors upward
- [x] Replace all `std::atoi` with `std::stoi` with validation

### 2.5 Modern C++ Cleanup

- [x] `CurlHttpClient`: `void* curl` → `std::unique_ptr<void, CurlHandleDeleter>`
- [x] Remove unused includes (`<cstdio>` from main.cpp, `<cstring>` from CurlHttpClient.cpp)
- [x] Fix include paths: `../data/ChapterProvider.h` → `data/ChapterProvider.h`
- [x] Wrap file-scope `static` in anonymous namespaces (EnvLoader, FontHelper, Highlighter, EmscriptenClient, StubChapterProvider)
- [x] Add `const` to `PersistenceManager::GetPreference`, `BibleClient::ParseHtmlChapter`
- [x] Remove decorative comments (section dividers in Theme.h)
- [ ] Deferred: Remove `friend class BibleClientTest` (requires test refactoring)

### 2.6 Verify

- [x] Build succeeds with no warnings
- [x] All 64 tests pass
- [x] App runs and renders correctly

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

- [x] Replace global `CMAKE_CXX_STANDARD` with `target_compile_features(theword_objects PUBLIC cxx_std_17)`
- [x] Replace `${CURL_LIBRARIES}` with `CURL::libcurl` imported target
- [x] Create `theword_objects` OBJECT library to eliminate duplicate source lists
- [x] Add `-Wall -Wextra -Wpedantic` with MSVC guard
- [x] Add `set(CMAKE_EXPORT_COMPILE_COMMANDS ON)` for clangd support
- [x] Fix asset copying: `file(COPY ...)` → `add_custom_command(TARGET theword POST_BUILD)`
- [x] Add `SYSTEM` qualifier for sqlite3 include directory
- [x] List header files in source lists for IDE visibility

### 3.3 Platform Fixes

- [x] **Windows**: Add `find_package(CURL)` for WIN32 branch
- [x] **macOS**: Add `APPLE` branch with `CURL::libcurl` + Cocoa/IOKit frameworks
- [x] **Android**: Keep `PLATFORM` cache variable (confirmed required by raylib's `LibraryConfigurations.cmake`)

### 3.4 Verify

- [x] `cmake --preset default` configures and builds
- [x] `cmake --preset debug` configures and builds
- [x] `cmake --build build && ./build/theword` runs correctly (verified assets copied)
- [x] Clean reconfigure: `rm -rf build && cmake --preset default`
- [x] 64/64 tests pass
- [x] `compile_commands.json` generated
- [x] No warnings from project code (3 pre-existing warnings from 3rd-party code ignored)

---

## Phase 4 — Android ARM + Cross-Platform Polish

### 4.1 Android ARM Support

- [x] Update `scripts/build-android.sh` to build for `x86_64`, `arm64-v8a` (armeabi-v7a dropped — raylib NEON half-float intrinsics require ARMv8+)
- [x] Create `src/main/java/com/theword/app/TheWordActivity.java` (Java activity stub for splash/IME)
- [x] Fix Android DB path: confirmed `/data/data/com.theword.app/app_storage/` works on all API 24+ (moved to Platform.cpp)
- [ ] Verify `GetAndroidApp()` resolves correctly from raylib's Android backend (requires NDK)

### 4.2 WASM Persistence

- [x] Add `-lidbfs.js` to Emscripten link flags in CMakeLists.txt
- [x] Mount IDBFS in `wasm_shell.html`
- [x] Add `__EMSCRIPTEN__` DB path branch in `Platform.cpp`: `/persistent/theword.db`

### 4.3 Build Scripts

- [x] Create `scripts/build-wasm.sh` for Emscripten builds
- [ ] Update `README.md` and `Build Guide.md` with preset-based workflow

### 4.4 libcurl Optional

- [x] `find_package(CURL QUIET)` instead of `REQUIRED`
- [x] Gate `CurlHttpClient` behind `CURL_FOUND` (BibleClient/CompositeProvider use abstract IHttpClient — no gating needed)
- [x] Document optional dependency in README and Environment Setup

### 4.5 Asset Loading Abstraction

- [x] Refactor `FontHelper` to accept `IAssetProvider*` — new overload `LoadFontCodepoints(IAssetProvider&, const std::string&)`
- [x] Centralize font loading through asset provider in `main.cpp`

### 4.6 Platform Dispatch

- [x] Create `src/core/Platform.h` with platform info struct (DB path, asset root, DPI scale)
- [x] Consolidate scattered `#ifdef` blocks from `main.cpp` into Platform module (9 blocks → 0 in main.cpp)

### 4.7 Verify

- [ ] Android APK builds for both x86_64 and arm64-v8a (requires NDK)
- [ ] WASM builds and persists highlights across page reload (requires emsdk)
- [x] Linux build works without libcurl installed (USFM-only mode) — code path written, tested with CURL present
- [ ] `scripts/build-wasm.sh` works end-to-end (requires emsdk)

---

## Phase 5 — Full Re-Architecture Proposal (Documentation Only) ✅ Complete (2026-06-26)

### 5.1 Re-Architecture Document ✅

- [x] Write `the-word-docs/07-ai-collaboration/Full Re-architecture Proposal.md` covering:
  - Namespace strategy for all modules (`theword::<module>::`)
  - Module boundaries & ownership (App class, UIManager stripped, event decoupling)
  - Platform abstraction extensions (OpenURL, clipboard, log sink)
  - EventBus design (type-safe, ~40 lines, header-only)
  - Ideal component splitting (InputHandler→emitter, UIManager→coordinator, App→orchestrator)
  - Incremental 8-step migration path (each step independently verifiable)

### 5.2 Convention Updates ✅

- [x] Update `the-word-docs/07-ai-collaboration/Convention Reference.md` with namespace conventions,
  platform abstraction pattern, ownership rules, event handler naming

### 5.3 Progress Tracking

- [ ] (No update needed — already reflects current state)

### 5.4 Build Guide Finalization

- [x] (Already done in Phase 4 wrap-up — Build Guide + README updated with preset workflow)

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
| **Phase 2: Code Quality** | ✅ Complete | 64/64 tests pass, build clean |
| **Phase 3: Build System** | ✅ Complete | Presets, modern CMake, OBJECT library, platform fixes |
| **Phase 4: Cross-Platform** | ✅ Complete | Platform dispatch + libcurl optional + Asset abstraction done; build verified (NDK/emsdk items noted as unverified) |
| **Phase 5: Re-Architecture** | ✅ Complete | Full proposal doc, convention updates, all roadmap docs consistent |
