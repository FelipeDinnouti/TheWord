# TheWord - Agent Development Guide

## Project Overview

**TheWord** is a minimalist Bible study application built on Raylib. Core features: Bible text rendering with line wrapping, infinite scroll, and per-word highlighting.

**Tech Stack:**
- Raylib 5.0 for rendering and window management
- libcurl for HTTP API calls
- C++17 with CMake build system
- YouVersion API for Bible text

---

## Build System

### Required Commands

```bash
# Configure (must use "Unix Makefiles" generator on Linux)
cmake -B build -DCMAKE_BUILD_TYPE=Release -G "Unix Makefiles"

# Build
cmake --build build --parallel

# Run
./build/theword
```

### CMake Caveats

1. **Always declare CXX language**: `project(theword C CXX)` — omitting `CXX` causes linker errors with some CMake versions.

2. **Generator choice**: Use `-G "Unix Makefiles"` on Linux. Default/ninja generators may fail with Raylib.

3. **Dependencies order**: libcurl's `find_package()` must come AFTER `FetchContent_MakeAvailable(raylib)` in CMakeLists.txt.

4. **New source files**: When adding new `.cpp` files, delete `build/` folder and reconfigure from scratch:
   ```bash
   rm -rf build && cmake -B build -DCMAKE_BUILD_TYPE=Release -G "Unix Makefiles"
   ```

5. **Platform-specific linking**:
   - Linux: links with `m`, `pthread`, `dl`, `rt`, X11 libraries
   - Windows: links with `opengl32`, `gdi32`, `winmm`

---

## Source Code Structure

```
src/
├── main.cpp              # Entry point, render loop
├── core/                 # Cross-cutting utilities
│   ├── Config.h          # Constants (window size, API URLs, defaults)
│   ├── APIClient.h/cpp   # HTTP client wrapper (libcurl)
│   └── EnvLoader.h/cpp   # .env file parser
├── data/                 # Data layer
│   ├── BibleVerse.h      # API response structures
│   └── BibleClient.h/cpp # Bible API client
├── text/                 # Layout engine
│   └── LayoutEngine.h/cpp # Text tokenization, word wrapping
├── document/             # Document management (not yet implemented)
├── highlight/            # Highlighting system (not yet implemented)
├── input/                # Input handling (not yet implemented)
├── persistence/          # SQLite persistence (not yet implemented)
└── renderer/             # UI rendering (not yet implemented)
```

### Coding Conventions

1. **Headers**: Use include guards (`#ifndef NAME_H / #define NAME_H / #endif`)
2. **Naming**:
   - Classes: `PascalCase`
   - Methods/functions: `PascalCase`
   - Variables: `camelCase`
   - Constants: `SCREAMING_SNAKE_CASE`
3. **Structures**: Define in header files, implement in `.cpp`
4. **Dependencies**: Keep downward (core → data → text → document → renderer)
5. **Raylib types**: Use `Vector2`, `Color`, `Font` from raylib.h

### File Organization

- One class per file: `ClassName.h` + `ClassName.cpp`
- Header files in `src/<module>/`
- No comments unless explaining non-obvious logic
- Keep public interfaces small and focused

---

## API Integration

### YouVersion API

**Base URL:** `https://api.youversion.com/v1`

**Authentication:** Header `X-YVP-App-Key: <your_key>`

**Key Endpoints:**
```
GET /bibles                                      # List available Bible versions
GET /bibles/{id}/passages/{usfm}?format=text     # Fetch verse text
```

**Bible Versions:**
- NIV (id 111): Requires special license, access denied for most apps
- BSB (id 3034): Works with standard app key — **use this as default**

### .env File Usage

1. Store secrets in `.env` file at project root (already gitignored)
2. Format: `KEY=value` (no spaces around `=`)
3. Supports `#` comments
4. Loaded by `EnvLoader::load(".env")` at startup

```env
YVP_APP_KEY=CgstASy4dMLafR6BZzkf34zDm0rHFc8dGiL5B6A69m4ZJ5nr
```

### API Client Pattern

```cpp
#include "core/EnvLoader.h"
#include "core/APIClient.h"
#include "data/BibleClient.h"

// Load .env and fetch API key
EnvLoader::load(config::ENV_FILE);
std::string appKey = EnvLoader::get(config::YVP_APP_KEY);

// Use BibleClient
BibleClient client(appKey);
BiblePassage passage = client.getPassage(3034, "JHN.3.16", "text");
```

---

## Text Layout Engine

### Core Concepts

1. **Tokenization**: Split raw text into `Word` structs with global ID and verse ID
2. **Word Wrapping**: Measure words with `MeasureTextEx()` and break lines at max width
3. **Span Creation**: Group words by verse into `Span` structs with document-space coordinates

### Key Structures

```cpp
struct Word {
    int id;           // Global word index (unique across document)
    int verseId;      // Verse reference
    std::string text; // The actual word
};

struct Span {
    std::string text;
    float x, y;       // Document-space position
    float width, height;
    int verseId;
    int startWord, endWord;
};

struct Line {
    float y;
    float height;
    std::vector<Span> spans;
};

struct ChapterLayout {
    std::string chapterId;
    float startY;
    float totalHeight;
    std::vector<Line> lines;
};
```

### Rendering Loop Pattern

```cpp
// In render loop
for (const auto& line : layout.lines) {
    float screenY = line.y - scrollY + contentTop;

    for (const auto& span : line.spans) {
        float textX = span.x;
        float textY = screenY + (line.height - span.height) / 2;

        DrawTextEx(font, span.text.c_str(), {textX, textY}, fontSize, 1, BLACK);
    }
}
```

---

## Window Configuration

Mobile-first aspect ratio:
- Width: 450px
- Height: 800px
- Target FPS: 60

```cpp
// In main.cpp
InitWindow(450, 800, "TheWord");
SetTargetFPS(60);
```

---

## Common Issues

| Issue | Solution |
|-------|----------|
| "CMAKE_CXX_COMPILE_OBJECT not set" | Add `CXX` to project languages: `project(theword C CXX)` |
| New .cpp files not compiled | Delete build folder, reconfigure CMake |
| libcurl not found | Install `libcurl4-openssl-dev`, ensure `find_package(CURL)` comes after raylib |
| API returns "Access denied" | Use Bible ID 3034 (BSB) instead of 111 (NIV) |
| API key not working | Verify `.env` file exists and `YVP_APP_KEY` is set correctly |

---

## Testing

### Manual Testing
1. Build: `cmake --build build --parallel`
2. Run: `./build/theword`
3. Verify: Window opens, text renders, FPS displays

### API Testing
```bash
# Test with curl
curl -H "X-YVP-App-Key: YOUR_KEY" \
  "https://api.youversion.com/v1/bibles/3034/passages/JHN.3.16?format=text"
```

### Fallback Behavior
If no API key is set, the app displays hardcoded fallback text (John 3:16).
This allows testing the layout engine without network access.

---

## Dependencies

### Build-time (installed via package manager)
- `cmake` >= 3.16
- `g++` with C++17 support
- `libcurl4-openssl-dev` (Linux)

### Runtime
- Raylib (downloaded via FetchContent during CMake configure)
- libcurl (system library)

### Raylib Dependencies (Linux)
```
libgl1-mesa-dev libx11-dev libxcursor-dev libxi-dev pkg-config
```

Install with: `sudo apt install build-essential cmake git libcurl4-openssl-dev libgl1-mesa-dev libx11-dev libxcursor-dev libxi-dev pkg-config`

---

## Development Workflow

1. **Feature implementation**: Create `ModuleName.h` + `ModuleName.cpp` in appropriate `src/` subdirectory
2. **Update CMake**: New source files auto-discovered via GLOB_RECURSE (no manual edits needed)
3. **Build**: `cmake --build build --parallel`
4. **Test**: `./build/theword`
5. **Document**: Update DEVELOPMENT_PLAN.md if adding significant functionality

### Phase Order
- Phase 1: Project Foundation ✓
- Phase 2: Text Layout Engine ✓
- Phase 3: Document Manager and Infinite Scroll
- Phase 4: Highlighting System
- Phase 5: USFM Parser
- Phase 6: SQLite Persistence
- Phase 7: UI Layer and Rendering
- Phase 8: Mobile Preparation (Android)

---

## Important Notes

1. **Coordinate Spaces**: Document space (0,0 = top of Genesis 1:1) vs Screen space (0,0 = window top-left). Convert by adding/subtracting scroll position from Y coordinate.

2. **Anchor-fixed prepend**: When prepending content, adjust scroll position by the height of prepended content so visible content stays anchored.

3. **Word IDs are global**: All words in the Bible are numbered sequentially (Genesis 1:1 words have lower IDs than Genesis 1:2 words). This makes range-based highlighting simple.

4. **Layout caching**: LayoutEngine caches ChapterLayout objects. Invalidate cache on window resize.

5. **libcurl initialization**: Create CURL handle once in constructor, cleanup in destructor. Use `curl_easy_setopt()` for configuration.

---

## Configuration Reference

Edit `src/core/Config.h` to change:

```cpp
namespace config {
    constexpr int WINDOW_WIDTH = 450;
    constexpr int WINDOW_HEIGHT = 800;
    constexpr int TARGET_FPS = 60;

    constexpr float FONT_SIZE = 20.0f;
    constexpr float LINE_SPACING = 1.5f;

    constexpr const char* API_BASE_URL = "https://api.youversion.com/v1";
    constexpr int DEFAULT_BIBLE_ID = 3034;  // BSB
    constexpr const char* DEFAULT_VERSE = "JHN.3.16";

    constexpr const char* ENV_FILE = ".env";
    constexpr const char* YVP_APP_KEY = "YVP_APP_KEY";
}
```