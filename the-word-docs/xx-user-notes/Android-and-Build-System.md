# Android Lifecycle & Build System Deep Dive

> Status: User Notes | Last Updated: 2026-06-24
> 
> This document explains two systems that are easy to lose track of when
> collaborating with AI agents: (1) how C++ code integrates with the Android
> lifecycle via NativeActivity and Raylib, and (2) how CMake and Ninja work
> together to build the project.

---

## Part 1: Android Lifecycle + C++ Integration

### 1.1 High-Level Picture

```
┌──────────────────────────────────────────────────────────────────┐
│ Android OS                                                       │
│  Launches Activity → loads libtheword.so                         │
└──────────────────────────┬───────────────────────────────────────┘
                           │
                           ▼
┌──────────────────────────────────────────────────────────────────┐
│ NativeActivity (android.app.NativeActivity)                      │
│  - No Java/Kotlin code (android:hasCode="false")                 │
│  - Provided by the Android OS itself                             │
│  - Calls android_main() defined in native_app_glue               │
└──────────────────────────┬───────────────────────────────────────┘
                           │
                           ▼
┌──────────────────────────────────────────────────────────────────┐
│ native_app_glue (NDK)                                            │
│  android_native_app_glue.c  (compiled by Raylib's CMake)         │
│     ↓ android_main(app)                                          │
│     ↓ stores android_app*                                        │
│     ↓ calls YOUR main()                                          │
└──────────────────────────┬───────────────────────────────────────┘
                           │
                           ▼
┌──────────────────────────────────────────────────────────────────┐
│ Raylib: rcore_android.c                                          │
│  - Defines android_main()                                        │
│  - Registers lifecycle callbacks                                 │
│  - Manages EGL context (OpenGL ES)                               │
│  - Exposes GetAndroidApp() for your code                         │
│  - Maps touch input → mouse events                               │
└──────────────────────────┬───────────────────────────────────────┘
                           │
                           ▼
┌──────────────────────────────────────────────────────────────────┐
│ Your C++ Code (src/main.cpp)                                     │
│  - main() runs inside android_main()                             │
│  - Calls InitWindow() → triggers InitPlatform()                  │
│  - Calls GetAndroidApp() for density, assets, paths              │
│  - Standard game loop: while(!WindowShouldClose()) { ... }       │
└──────────────────────────────────────────────────────────────────┘
```

### 1.2 Entry Point Chain

**Step 1 — AndroidManifest.xml** (`AndroidManifest.xml:15-17`):

```xml
<activity
    android:name="android.app.NativeActivity"
    android:hasCode="false">
    <meta-data android:name="android.app.lib_name" android:value="theword"/>
</activity>
```

`android:hasCode="false"` means the activity has no Java bytecode. The OS
loads `libtheword.so` and calls its native entry point. The `lib_name`
meta-data tells the OS the shared library name (without `lib` prefix or
`.so` suffix).

**Step 2 — OS calls `android_main()`** — defined in Raylib's
`rcore_android.c:104`:

```c
void android_main(struct android_app *app)
{
    char arg0[] = "raylib";
    platform.app = app;  // ← saves the android_app pointer globally

    // Raylib calls YOUR main() directly from here!
    (void)main(1, (char *[]) { arg0, NULL });

    // After main() returns, request activity finish
    ANativeActivity_finish(app->activity);

    // Wait for the OS to confirm destruction
    while (!app->destroyRequested)
    {
        ALooper_pollAll(0, NULL, &pollEvents, (void**)&platform.source);
        if (platform.source != NULL)
            platform.source->process(app, platform.source);
    }
}
```

Your `main()` in `src/main.cpp` is **not** called by the OS's libc — it is
called directly by Raylib's `android_main()`. This is the central trick.

**Step 3 — `GetAndroidApp()` gives your code access to the `android_app*`**
(`rcore_android.c:130`):

```c
struct android_app *GetAndroidApp(void)
{
    return platform.app;
}
```

Your `main.cpp` declares it with:
```cpp
extern "C" struct android_app* GetAndroidApp(void);
```

Then uses it for three things:

1. **Screen density** — to scale fonts and window size for high-DPI displays:
```cpp
int density = AConfiguration_getDensity(GetAndroidApp()->config);
float densityScale = (density > 0) ? (float)density / 160.0f : 1.0f;
```

2. **Asset manager** — to read bundled files (USFM, fonts, `.env`) from the
   APK's assets directory instead of the filesystem:
```cpp
AAssetManager* mgr = GetAndroidApp()->activity->assetManager;
AndroidAssetProvider androidAssets(mgr);
```

3. **Database path** — the app-specific storage directory:
```cpp
std::string dbPath = "/data/data/com.theword.app/app_storage/" + config::DB_FILE;
```

### 1.3 Lifecycle Callbacks

Raylib registers `AndroidCommandCallback` inside `InitPlatform()`
(`rcore_android.c:561`):

```c
platform.app->onAppCmd = AndroidCommandCallback;
```

This callback handles every Android lifecycle event:

| Event | What Happens |
|---|---|
| `APP_CMD_INIT_WINDOW` | EGL display created, OpenGL ES context initialized, graphics device set up, default font loaded, timer initialized. **First call = full init. Subsequent calls = context rebind** (after `APP_CMD_TERM_WINDOW`) |
| `APP_CMD_GAINED_FOCUS` | Sets `platform.appEnabled = true` → the event loop unblocks and runs normally |
| `APP_CMD_LOST_FOCUS` | Sets `platform.appEnabled = false` → the event loop **blocks** on `ALooper_pollAll(-1)` indefinitely |
| `APP_CMD_TERM_WINDOW` | Detaches EGL context and destroys window surface. Sets `contextRebindRequired = true` for graceful resume |
| `APP_CMD_PAUSE` / `APP_CMD_RESUME` | Currently no-ops in Raylib (reserved for music streaming) |
| `APP_CMD_CONFIG_CHANGED` | Fires on rotation. Currently logs only (no re-layout). You'd handle this in your code |

The blocking behavior of the main loop is implemented in `PollInputEvents()`
(`rcore_android.c:497-510`):

```c
while ((pollResult = ALooper_pollAll(
    platform.appEnabled ? 0 : -1,  // ← blocks when not focused!
    NULL, &pollEvents, (void**)&platform.source)) >= 0)
{
    if (platform.source != NULL)
        platform.source->process(platform.app, platform.source);
}
```

When `platform.appEnabled` is `false`, the timeout is `-1`, which means
`ALooper_pollAll` **blocks forever** — your game loop thread literally
halts. When the OS sends `APP_CMD_GAINED_FOCUS`, the callback sets
`appEnabled = true`, the poll returns immediately with whatever events
are queued, and your loop continues.

### 1.4 Touch Input

Raylib's `AndroidInputCallback` (`rcore_android.c:925`) maps touch events:

1. Reads all touch points from `AMotionEvent`
2. Maps `touch[0]` → `CORE.Input.Mouse.currentPosition` (touch acts as mouse)
3. Sets `CORE.Input.Touch.currentTouchState` → `MOUSE_BUTTON_LEFT`
4. Passes events to Raylib's gesture system

From your code's perspective: touch is a mouse click at the touch position.
The `InputHandler` in `src/input/InputHandler.cpp` calls `GetMouseWheelMove()`
and `IsKeyDown()` etc., which work transparently because Raylib has already
translated the Android input events.

Two keyboard keys are special-cased:
- `AKEYCODE_BACK` / `AKEYCODE_MENU` — **eaten** (return 1), so the OS doesn't
  use them for navigation
- `AKEYCODE_POWER` — **passed through** (return 0), so the OS handles sleep

### 1.5 EGL Context Management

When the app goes to background and comes back:

```
APP_CMD_TERM_WINDOW:
    eglMakeCurrent(device, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT)
    eglDestroySurface(device, surface)
    contextRebindRequired = true

APP_CMD_INIT_WINDOW (on resume):
    if (contextRebindRequired):
        ANativeWindow_setBuffersGeometry(...)
        surface = eglCreateWindowSurface(...)
        eglMakeCurrent(device, surface, surface, context)
        contextRebindRequired = false
    else (first launch):
        EGL full initialization
```

The important thing: **OpenGL resources (textures, VBOs) are NOT automatically
reloaded** after context rebind. Raylib has TODO comments about this. If you
loaded custom GPU textures, you'd need to reload them on context rebind.

### 1.6 Current State in the Project

The Android build is **in-progress** (Phase 8 in the development plan).
Several pieces are stubbed or incomplete:

- `native_app_glue.c` is **not explicitly compiled** in CMakeLists.txt
  (Raylib's CMake should handle this, but the toolchain must be set correctly)
- `-Wl,-undefined,dynamic_lookup` means unresolved symbols won't fail at link
  time but will crash at runtime
- The `InputHandler` assumes mouse + keyboard; touch works only because Raylib
  maps it, but there's no pinch-to-zoom or swipe
- Font atlases and other GPU resources would need reloading on EGL context
  rebind (Raylib handles default font, but custom `bodyFont` / `headingFont`
  may need re-creation)

---

## Part 2: CMake & Ninja Build System

### 2.1 The Meta-Build Concept

CMake is a **meta-build system** — it does not compile anything. Instead, it
reads `CMakeLists.txt` and generates input files for a **backend build tool**:

```
CMakeLists.txt (declarative) → CMake (configure)
    │
    ├──→ Makefile          (GNU Make)
    ├──→ build.ninja       (Ninja)
    ├──→ .sln / .vcxproj   (MSVC / Visual Studio)
    └──→ .xcodeproj        (Xcode)
```

You pick the backend with `-G`:
```bash
cmake -B build -G "Unix Makefiles"   # GNU Make (current)
cmake -B build -G Ninja              # Ninja
```

The same `cmake --build build` command works regardless of backend.

### 2.2 Configuration Phase (cmake -B build)

When you run `cmake -B build [...]`, CMake:

**1. Reads the project declaration**
```cmake
cmake_minimum_required(VERSION 3.16)
project(theword C CXX)           # ← note: both C and CXX required!
set(CMAKE_CXX_STANDARD 17)
```

**2. Fetches dependencies** via `FetchContent`

Each `FetchContent_Declare` + `FetchContent_MakeAvailable` pair:
- Downloads the source tarball
- Caches it in `build/_deps/<name>-src/`
- Runs CMake on the dependency's own `CMakeLists.txt` (recursive)
- Build artifacts go to `build/_deps/<name>-build/`

```
FetchContent_Declare(raylib URL ...)
FetchContent_MakeAvailable(raylib)
    → downloads raylib-5.0.tar.gz → extracts to build/_deps/raylib-src/
    → runs cmake on build/_deps/raylib-src/CMakeLists.txt
    → raylib becomes a CMake target you can link against
```

The same process applies to `doctest` and `sqlite3`.

**3. Selects platform-specific source files** via conditionals:

```cmake
if(EMSCRIPTEN)
    set(CORE_SOURCES src/core/EmscriptenClient.cpp ...)
elseif(ANDROID)
    set(CORE_SOURCES src/core/AndroidClient.cpp
                     src/core/AndroidAssetProvider.cpp ...)
else()
    set(CORE_SOURCES src/core/CurlHttpClient.cpp ...)
endif()
```

On Android, it also builds `libcurl` from source via `add_subdirectory` on the
fetched curl tarball.

**4. Creates targets**

```cmake
if(ANDROID)
    add_library(theword SHARED ${THEWORD_SOURCES})   # → libtheword.so
else()
    add_executable(theword ${THEWORD_SOURCES})        # → theword binary
endif()
```

- On Android: `SHARED` library (`.so`) because NativeActivity loads a shared
  library, not an executable.
- On desktop: executable.

A second target `theword_test` is created on desktop only, linked with doctest.

**5. Configures platform-specific linking**

| Platform | Libraries Linked |
|---|---|
| Linux | `raylib` + `curl` + `m pthread dl rt` + `X11 Xcursor Xrandr Xi` |
| Android | `raylib` + `libcurl_static` + `android` + `EGL GLESv2 log` |
| WASM | `raylib` only + emscripten linker flags |
| Windows | `raylib` + `opengl32 gdi32 winmm` |

**6. Writes `build/CMakeCache.txt`** — a cache of all variable values. On
subsequent configures, CMake reads this instead of re-evaluating everything.
Delete `build/` to force a full reconfigure.

**7. Writes the backend build file** — `build/Makefile` or `build/build.ninja`.

### 2.3 Build Phase (cmake --build build --parallel)

This calls the backend tool:

```bash
# With Makefiles, equivalent to:
make -C build -j$(nproc)

# With Ninja, equivalent to:
ninja -C build -j$(nproc)
```

Each `.cpp` file becomes a **compile step**:

```
src/main.cpp → build/src/main.cpp.o
    g++ -c -std=c++17 -I src -I build/_deps/raylib-src/src ...
        -o build/src/main.cpp.o src/main.cpp
```

All `.o` files are then **linked** into the final binary.

The `--parallel` flag tells the backend to use all CPU cores. Ninja is
particularly good at this because:
- It was designed from scratch for maximum parallelism
- It has no recursive invocations (unlike Make with subdirectories)
- Dependency edges are explicit in the `.ninja` file, so Ninja can start
  compiling independent files immediately without analyzing Makefile targets

### 2.4 What the Generated Build File Looks Like

**Makefile** (`build/Makefile` — simplified):

```makefile
src/core/Logger.cpp.o: src/core/Logger.cpp
    g++ $(CXXFLAGS) -c src/core/Logger.cpp -o src/core/Logger.cpp.o

src/main.cpp.o: src/main.cpp
    g++ $(CXXFLAGS) -c src/main.cpp -o src/main.cpp.o

theword: src/main.cpp.o src/core/Logger.cpp.o ...
    g++ src/main.cpp.o src/core/Logger.cpp.o ... -o theword -lraylib
```

**Ninja** (`build/build.ninja` — simplified):

```ninja
rule cc
  command = g++ $CXXFLAGS -c $in -o $out

build src/core/Logger.cpp.o: cc src/core/Logger.cpp
build src/main.cpp.o: cc src/main.cpp
build theword: link src/main.cpp.o src/core/Logger.cpp.o ...
  libs = -lraylib
```

### 2.5 Ninja vs Make

| Aspect | GNU Make | Ninja |
|---|---|---|
| Purpose | General-purpose build tool | **Fast, minimal** build tool |
| Language | Complex DSL (macros, functions, conditionals) | Simple flat format (rules + edges) |
| Parallelism | Good, but recursive make hurts | **Excellent** — designed for parallelism |
| Incremental builds | Good (checks timestamps) | **Better** — faster at determining what changed |
| Autoconf support | Yes | Not needed (CMake generates for you) |
| File size | Large generated Makefile | **Compact** `.ninja` file |
| Who writes it | Often human + autotools | **Never human** — always generated (by CMake, Meson, etc.) |

The practical difference: with Make, `cmake --build build` takes ~1-2 seconds
just to start compiling on large projects because Make reads and parses the
entire Makefile tree. Ninja starts compiling in milliseconds.

### 2.6 Common CMake Patterns in This Project

**Object library** — compiled once, linked to multiple targets:

```cmake
add_library(sqlite3_obj OBJECT ${sqlite3_SOURCE_DIR}/sqlite3.c)
# ...
$<TARGET_OBJECTS:sqlite3_obj>  # in both theword and theword_test
```

The `$<TARGET_OBJECTS:...>` generator expression is evaluated at CMake
generate-time, not at build time. It inserts the `.o` files from the object
library into both targets without compiling SQLite twice.

**Generator expressions** — evaluated during generation (not configure):

```cmake
$<TARGET_OBJECTS:sqlite3_obj>
```

**FetchContent caching** — dependencies are fetched once:

```
build/_deps/
├── raylib-src/      ← source (fetched once)
├── raylib-build/    ← build artifacts
├── doctest-src/
├── doctest-build/
├── sqlite3-src/
└── sqlite3-build/
```

Delete `build/` to force re-fetch all dependencies.

**Asset copying** — files are copied to the build directory:

```cmake
file(COPY assets DESTINATION ${CMAKE_BINARY_DIR})
```

This runs during **configure**, not build. If you modify assets, you need to
reconfigure (or copy them manually).

### 2.7 Adding a New .cpp File

Because the project uses explicit file lists (not `GLOB_RECURSE`), you must
add every new `.cpp` to `CMakeLists.txt` and then reconfigure:

```bash
rm -rf build && cmake -B build -DCMAKE_BUILD_TYPE=Release -G "Unix Makefiles"
```

This is intentional — `GLOB_RECURSE` is discouraged because CMake won't
automatically detect new files (it caches the file list).

### 2.8 Building for Different Platforms

The same `CMakeLists.txt` handles all targets. You just change the configure
flags:

```bash
# Desktop Linux (current)
cmake -B build -DCMAKE_BUILD_TYPE=Release -G "Unix Makefiles"

# Android (planned)
cmake -B build-android \
    -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
    -DANDROID_ABI=arm64-v8a \
    -DANDROID_PLATFORM=android-24 \
    -DANDROID=ON \
    -G Ninja

# WebAssembly (planned)
emcmake cmake -B build-wasm -DEMSCRIPTEN=ON -G Ninja
```

CMake's `if(ANDROID)` / `if(EMSCRIPTEN)` conditionals in `CMakeLists.txt`
respond to variables set by the toolchain files or passed via `-D`.

---

## Appendix: Key Source Files Referenced

| File | Role |
|---|---|
| `src/main.cpp` | Entry point (`main()`) — platform agnostic except `#ifdef __ANDROID__` blocks |
| `src/core/AndroidClient.cpp` | HTTP client using libcurl (static-linked for Android) |
| `src/core/AndroidAssetProvider.cpp` | Reads files from APK assets via `AAssetManager` |
| `AndroidManifest.xml` | Declares `NativeActivity`, permissions, lib name |
| `CMakeLists.txt` | All build configuration |
| `build/_deps/raylib-src/src/platforms/rcore_android.c` | Raylib's Android platform — `android_main()`, `GetAndroidApp()`, lifecycle callbacks, EGL, touch input |
