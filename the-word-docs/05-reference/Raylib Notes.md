# Raylib Notes

> Status: Reference | Last Updated: 2026-06-21

## Version

Raylib 5.0 (via FetchContent in CMake).

## Key APIs Used

- `InitWindow()`, `CloseWindow()`, `WindowShouldClose()` — Window management
- `SetTargetFPS()` — Frame rate control
- `LoadFontEx()`, `UnloadFont()` — Font management (used with explicit codepoint vectors for non-ASCII glyphs)
- `SetTextureFilter()` — Texture filtering mode (`TEXTURE_FILTER_POINT` for pixel-sharp font rendering at 1:1)
- `MeasureTextEx()` — Text measurement for word wrapping
- `DrawTextEx()` — Text rendering (used for all Bible text with the appropriate font atlas)
- `DrawText()` — Simple text rendering for UI (not used for Bible text)
- `DrawRectangle()` — Scrollbar and highlight rectangles
- `GetMouseWheelMove()` — Scroll input
- `IsKeyDown()` — Keyboard input
- `GetScreenWidth()`, `GetScreenHeight()` — Window dimensions
- `BeginDrawing()`, `EndDrawing()` — Render loop
- `ClearBackground()` — Background fill
- `DrawFPS()` — Debug overlay

## Types Used

- `Font` — Font resource
- `Vector2` — 2D coordinates and sizes
- `Color` — RGBA colors

## FetchContent Configuration

```cmake
FetchContent_Declare(
    raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/5.0.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

set(BUILD_GAMES OFF)
set(BUILD_EXAMPLES OFF)

FetchContent_MakeAvailable(raylib)
```

## Platform-Specific Linking

**Linux:**
```cmake
target_link_libraries(theword PRIVATE
    raylib
    ${CURL_LIBRARIES}
    m pthread dl rt
    X11 Xcursor Xrandr Xi
)
```

**Windows:**
```cmake
target_link_libraries(theword PRIVATE
    raylib
    opengl32 gdi32 winmm
)
```

## Font Atlas Strategy

The app uses **two font atlases** to achieve pixel-sharp text at multiple sizes:

1. **bodyFont** — Generated at `config::FONT_SIZE` (24px), used for verse text and poetry lines (1:1 render)
2. **headingFont** — Generated at `config::FONT_HEADING_SIZE` (31px), used for section headings, chapter labels, book titles, and the chapter title bar (1:1 render)

Both use `TEXTURE_FILTER_POINT` since there is no scaling — each atlas matches its display size exactly.

Codepoints are loaded explicitly to cover Portuguese accents:
```cpp
std::vector<int> codepoints;
for (int i = 32; i < 127; i++) codepoints.push_back(i);   // ASCII printable
for (int i = 160; i < 256; i++) codepoints.push_back(i);   // Latin-1 Supplement
```

## Important Note

The `find_package(CURL)` call must come AFTER `FetchContent_MakeAvailable(raylib)` in CMakeLists.txt. If placed before, CMake may fail to find resolved dependencies.
