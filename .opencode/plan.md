# Phase 10: Mobile Preparation — Detailed Plan

> Status: Draft | Last Updated: 2026-06-23
> Target: Emscripten (WASM) → Touch Gestures → Android NDK

## Overview

Refactor `APIClient` into a polymorphic `IHttpClient` interface so that HTTP
requests work on all platforms without losing online capability.  Then build
WASM (Emscripten) first, add touch gestures, and finally target Android NDK.

### Architecture Change

```
Before:                     After:
APIClient (concrete)        IHttpClient (abstract interface, same `get(url)`)
  └─ libcurl                   ├─ CurlHttpClient     [desktop:   libcurl]
                               ├─ EmscriptenClient   [WASM:      emscripten_fetch]
                               └─ AndroidClient      [Android:   bundled libcurl]
```

**Key invariant**: `BibleClient` holds `IHttpClient&` — zero changes to
`BibleClient.h/cpp`.

---

## Step 1 — IHttpClient Refactor

**Goal**: Turn APIClient into a polymorphic interface.  Desktop still uses
libcurl.  Tests use a mock.

### Files to Create

| File | Purpose |
|------|---------|
| `src/core/IHttpClient.h` | Abstract interface |
| `src/core/CurlHttpClient.h` | Desktop impl (moved from APIClient) |
| `src/core/CurlHttpClient.cpp` | Desktop impl (moved from APIClient) |

### Files to Modify

| File | Change |
|------|--------|
| `src/core/APIClient.h` | Remove; contents moved to IHttpClient + CurlHttpClient |
| `src/core/APIClient.cpp` | Remove; contents moved to CurlHttpClient.cpp |
| `CMakeLists.txt` | Replace `APIClient.cpp` → `CurlHttpClient.cpp`, remove `curl/curl.h` dep from interface |
| `src/data/BibleClient.h` | `#include "core/APIClient.h"` → `#include "core/IHttpClient.h"`, `APIClient&` → `IHttpClient&` |
| `src/data/BibleClient.cpp` | No changes (already uses `apiClient.get(url)`) |
| `src/main.cpp` | `APIClient` → `CurlHttpClient` |
| `tests/test_main.cpp` | `#include "core/APIClient.h"` → `#include "core/IHttpClient.h"` |

### What IHttpClient Looks Like

```cpp
// src/core/IHttpClient.h
#ifndef IHTTPCLIENT_H
#define IHTTPCLIENT_H

#include <string>

class IHttpClient {
public:
    virtual ~IHttpClient() = default;
    virtual std::string get(const std::string& url) = 0;
    virtual void setAppKey(const std::string& key) = 0;
    virtual std::string getAppKey() const = 0;
};

#endif
```

### What CurlHttpClient Looks Like

```cpp
// src/core/CurlHttpClient.h
#ifndef CURLHTTPCLIENT_H
#define CURLHTTPCLIENT_H

#include "IHttpClient.h"

class CurlHttpClient : public IHttpClient {
public:
    CurlHttpClient();
    ~CurlHttpClient() override;
    // no copy, default move
    std::string get(const std::string& url) override;
    void setAppKey(const std::string& key) override;
    std::string getAppKey() const override;
private:
    void* curl;  // opaque — curl included in .cpp only
    std::string appKey;
};

#endif
```

`CurlHttpClient.cpp` is the existing `APIClient.cpp` unchanged, only the class
name changes and `#include <curl/curl.h>` moves to the .cpp.

### Test Changes

- Create `src/core/MockHttpClient.h` (test utility):
  ```cpp
  class MockHttpClient : public IHttpClient {
      std::string response;
  public:
      void setResponse(const std::string& r) { response = r; }
      std::string get(const std::string&) override { return response; }
      void setAppKey(const std::string&) override {}
      std::string getAppKey() const override { return ""; }
  };
  ```
- BibleClient tests use `MockHttpClient` instead of `APIClient` — no more real
  network dependency in tests.
- Place in `tests/` directory (not in `src/`).

### CMakeLists.txt Changes

```cmake
set(CORE_SOURCES
    src/core/CurlHttpClient.cpp    # was APIClient.cpp
    src/core/EnvLoader.cpp
    src/core/FontHelper.cpp
)
# find_package(CURL) still REQUIRED on desktop (inside if(EMSCRIPTEN)/else block)
```

### Verification

- Build desktop: `cmake --build build && ./build/theword`
- All 64 tests pass
- Bible still loads from both USFM and online (if API key present)

---

## Step 2 — Emscripten/WASM Build

**Goal**: App runs in browser with full online support via `emscripten_fetch`.

### Prerequisites

Install Emscripten SDK:
```bash
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk && ./emsdk install latest && ./emsdk activate latest
source ./emsdk_env.sh
```

### Files to Create

| File | Purpose |
|------|---------|
| `src/core/EmscriptenClient.h` | WASM HTTP client |
| `src/core/EmscriptenClient.cpp` | Uses `emscripten_fetch()` with sync flag |

### EmscriptenClient Implementation

```cpp
// Uses emscripten_fetch with EMSCRIPTEN_FETCH_SYNCHRONOUS
// Wraps the async browser fetch into a blocking call.
// See: https://emscripten.org/docs/api_reference/fetch.html
```

Key details:
- `emscripten_fetch()` with synchronous flag blocks until done
- Set `X-YVP-App-Key` header via `__EMSCRIPTEN_FETCH_HEADER__` mechanism or
  manual HTTP headers
- Returns empty string on failure (same contract as CurlHttpClient)

### CMakeLists.txt Additions

```cmake
if(EMSCRIPTEN)
    add_executable(theword ...)
    target_link_options(theword PRIVATE
        -s USE_GLFW=3
        -s ASYNCIFY
        --preload-file assets
        -s TOTAL_MEMORY=64MB
        -s MAX_WEBGL_VERSION=2
        --shell-file wasm_shell.html
    )
    # no find_package(CURL), no -lX11 etc.
elseif(ANDROID)
    # (Step 4)
else()
    # desktop: find_package(CURL) as today
endif()
```

### New Files

`wasm_shell.html` — Minimal HTML shell:
```html
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=450, initial-scale=1.0">
    <title>TheWord</title>
</head>
<body style="margin:0; background:#1e1e1e; display:flex; justify-content:center;">
    <canvas id="canvas" style="width:450px; height:800px;"></canvas>
</body>
</html>
```

### Asset Handling

- USFM files: `--preload-file assets/usfm` — Emscripten loads into virtual FS,
  `ifstream` works unchanged
- Fonts: `--preload-file assets/fonts` — `LoadFontEx()` works via Raylib's
  internal asset mapping
- Memory FS is read-only at runtime for preloaded files

### main.cpp Awareness

```cpp
#ifdef __EMSCRIPTEN__
    #include "core/EmscriptenClient.h"
    EmscriptenClient httpClient;
#else
    #include "core/CurlHttpClient.h"
    CurlHttpClient httpClient;
#endif
```

### Input for WASM

Browser input maps automatically via Raylib:
- `GetMousePosition()` → mouse/touch on canvas
- `GetMouseWheelMove()` → scroll events
- `IsKeyDown()` → keyboard events
- Touch gestures **not yet refined** (Step 3)

### Build & Serve

```bash
emcmake cmake -B build-wasm -DCMAKE_BUILD_TYPE=Release -G "Ninja"
cmake --build build-wasm --parallel
# Serve (Emscripten generates .html + .wasm + .js)
emrun build-wasm/theword.html
```

### Verification

- App loads in browser at `localhost:6931`
- USFM offline chapters display correctly (Genesis 1)
- Online API chapters display correctly when API key configured
- Splash screen shows, navigation works, settings work
- Keyboard shortcuts (G, S, A, Escape, arrows) work

---

## Step 3 — Touch Gesture System

**Goal**: Swipe scrolls, tap taps, long-press → context menu, pinch → zoom.

### Files to Modify

| File | Change |
|------|--------|
| `src/input/InputHandler.h` | Add `handleTouchInput()`, `handleDesktopInput()` or platform `#ifdef` branches |
| `src/input/InputHandler.cpp` | Split input polling into platform paths |

### Strategy

**Option A (recommended)**: Polymorphic input backend.

```cpp
// InputHandler.h
class InputHandler {
    void handleInput(float deltaTime);
    struct TouchState {
        Vector2 pos;
        bool pressed;
        bool released;
        bool down;
        int wheelDelta;
        bool keyPressed(int key);
    };
    TouchState readInput();  // platform-specific
};
```

**Option B**: `#ifdef` branches (simpler but less clean). For now, use
`#ifdef` since we know the exact 3 platforms.

```cpp
void InputHandler::handleInput(float deltaTime) {
    // Dialog routing (unchanged)
    if (uiManager.isAboutActive()) { ... }
    // ...

#ifdef __EMSCRIPTEN__  // or __ANDROID__
    handleTouchInput();
#else
    handleDesktopInput();
#endif
}
```

### Touch Gesture Mapping

| Gesture | Detection | Action |
|---------|-----------|--------|
| **Swipe up/down** | `GetTouchY()` delta over frames | Scroll (with momentum) |
| **Tap** (finger down < 300ms, move < 10px) | Same FSM as mouse left-click | Highlight word |
| **Long-press** (finger down > 500ms, move < 10px) | Existing `PressState::LongPress` | Context menu |
| **Pinch** | Two-finger distance delta | Font size zoom (A–/A+) |
| **Drag** (finger down + move > 10px) | Existing `PressState::Dragging` | Selection |

### Fling/Momentum Scroll

- On swipe release, set `scrollVelocity` proportional to last frame's delta-Y
- Existing friction decay already handles deceleration
- Tune constants: `SCROLL_SENSITIVITY` for touch (different from mouse wheel)

### Gesture Detection (Pinch)

```cpp
void InputHandler::handlePinch() {
    int touchCount = GetTouchPointCount();
    if (touchCount >= 2) {
        Vector2 t1 = GetTouchPosition(0);
        Vector2 t2 = GetTouchPosition(1);
        float dist = sqrt(pow(t2.x - t1.x, 2) + pow(t2.y - t1.y, 2));
        static float lastDist = 0;
        if (lastDist > 0) {
            float delta = dist - lastDist;
            if (delta > 5) uiManager.changeFontSize(config::FONT_SIZE_STEP);
            if (delta < -5) uiManager.changeFontSize(-config::FONT_SIZE_STEP);
        }
        lastDist = dist;
    } else {
        lastDist = 0;
    }
}
```

### Go-To Dialog Keyboard on Mobile

- WASM: browser shows virtual keyboard on `<input>` focus — but we use
  Raylib's `GetCharPressed()`, which works on WASM if the canvas has focus
- Android: show soft keyboard via JNI or Raylib's Android keyboard support

### Verification

- Test on WASM with mobile viewport in Chrome DevTools
- Swipe scrolls smoothly with momentum
- Tap highlights words
- Long-press shows context menu
- Pinch (if two-finger available in emulation) changes font size
- All existing desktop input still works (regression test)

---

## Step 4 — Android NDK Build + Lifecycle

**Goal**: App runs on Android device/emulator with online support and proper
lifecycle handling.

### Prerequisites

```bash
# Install Android SDK + NDK (e.g. via Android Studio or sdkmanager)
sdkmanager "ndk;25.2.9519653" "platforms;android-24"
export ANDROID_NDK=$HOME/Android/Sdk/ndk/25.2.9519653
```

### Files to Create

| File | Purpose |
|------|---------|
| `src/core/AndroidClient.h` | Android HTTP client |
| `src/core/AndroidClient.cpp` | Uses bundled libcurl-for-NDK |
| `src/core/AndroidAssetProvider.h` | Asset loader for APK |
| `src/core/AndroidAssetProvider.cpp` | Wraps `AAssetManager` |
| `AndroidManifest.xml` | App manifest |
| `scripts/build-android.sh` | Build + package script |
| `assets/icon.png` | App icon (48×48, 96×96) |

### Files to Modify

| File | Change |
|------|--------|
| `src/core/USFMParser.h` | Accept `AssetProvider*` for file reads |
| `src/core/USFMParser.cpp` | Use `AssetProvider` if set, else `ifstream` |
| `src/main.cpp` | `#ifdef __ANDROID__` entry point + lifecycle |
| `CMakeLists.txt` | `if(ANDROID)` block with NDK toolchain + bundled curl |
| `src/persistence/PersistenceManager.cpp` | Android path via `getDataDir()` |

### AndroidClient

- Bundles libcurl from source (via FetchContent):
  ```cmake
  FetchContent_Declare(curl
      URL https://curl.se/download/curl-8.4.0.tar.gz
  )
  set(CURL_USE_OPENSSL OFF)
  set(CURL_DISABLE_CRYPTO_AUTH ON)
  set(ENABLE_ARES OFF)
  FetchContent_MakeAvailable(curl)
  ```
- Or use a prebuilt NDK libcurl port
- Implements `IHttpClient` identically to `CurlHttpClient`

### AssetProvider Interface

```cpp
// src/core/IAssetProvider.h (new)
class IAssetProvider {
public:
    virtual ~IAssetProvider() = default;
    virtual std::optional<std::string> readFile(const std::string& path) = 0;
};
```

Implementations:
- `FileAssetProvider` (desktop/WASM): reads via `ifstream`
- `AndroidAssetProvider`: reads via `AAssetManager_open()` + `AAsset_getBuffer()`

### USFMParser Refactor

```cpp
// USFMParser.h
class USFMParser : public ChapterProvider {
public:
    USFMParser(const std::string& usfmDir, IAssetProvider* assets = nullptr);
    // ...
private:
    IAssetProvider* assets;  // null → use ifstream (desktop/WASM)
};
```

When `assets` is non-null, USFMParser reads through it instead of `ifstream`.

### Android Entry Point

Raylib 5.0 provides `android_app_init()` wrapper internally.  Our `main.cpp`
may need a thin wrapper:

```cpp
#ifdef __ANDROID__
// Raylib handles android_app_init; our main() is called from native
// activity.  Currently no change needed for Raylib 5.0.
#endif
```

However, we need lifecycle hooks:

```cpp
void onPause() {
    storage.setPreference("scroll_pos", std::to_string(docManager.getScrollY()));
    storage.setPreference("current_chapter", docManager.getCurrentChapterId());
}

void onResume() {
    // Reload fonts (OpenGL context lost)
    // Reload textures
    // Restore scroll position
    // Restore current chapter
}
```

### SQLite on Android

- Path: `getenv("HOME")` doesn't exist on Android
- Use `#ifdef __ANDROID__`:
  ```cpp
  #ifdef __ANDROID__
      // /data/data/com.theword/app_storage/highlights.db
      dbPath = std::string("/data/data/com.theword/app_storage/") + config::DB_FILE;
  #else
      dbPath = home + "/" + config::DB_DIR + "/" + config::DB_FILE;
  #endif
  ```

### APK Packaging

```bash
cmake -B build-android \
    -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
    -DANDROID_ABI=arm64-v8a \
    -DANDROID_PLATFORM=android-24 \
    -DCMAKE_BUILD_TYPE=Release \
    -G "Ninja"
cmake --build build-android --parallel
```

Then package into APK (Raylib provides some tooling, or use
`add_custom_target` with `aapt` + `apksigner`).

### Lifecycle Save/Restore

Save on pause:
- Current chapter ID (`getCurrentChapterId()`)
- Scroll position (`getScrollY()`)
- Active font size
- Open dialogs state

Restore on resume:
- Re-initialize subsystems (fonts, textures)
- Load saved chapter
- Scroll to saved position
- Restore font size

### Verification

- App launches on Android emulator (API 24+)
- Genesis 1 displays from USFM (offline)
- Online chapter loads (if API key configured in preferences)
- Touch input works (swipe, tap, long-press, pinch)
- App survives pause/resume cycle
- Highlights persist after restart
- Font size persists after restart

---

## Plan Review Checklist

Before starting Step 1:

| Check | Status |
|-------|--------|
| IHttpClient interface defined | ⬜ |
| BibleClient uses IHttpClient& | ⬜ |
| CurlHttpClient impl built & tested | ⬜ |
| 64/64 tests pass | ⬜ |
| WASM build command known | ⬜ |
| Emscripten SDK installed | ⬜ |
| Touch gesture mapping defined | ⬜ |
| Android NDK path known | ⬜ |
| AssetProvider interface defined | ⬜ |
| Lifecycle save/restore points identified | ⬜ |
| Updated Progress Tracking.md | ⬜ |
| Updated Development Plan.md | ⬜ |
