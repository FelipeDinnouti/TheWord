# Raylib Notes

> Status: Reference | Last Updated: 2026-06-21

## Version

Raylib 5.0 (via FetchContent in CMake).

## Key APIs Used

- `InitWindow()`, `CloseWindow()`, `WindowShouldClose()` — Window management
- `SetTargetFPS()` — Frame rate control
- `LoadFontEx()`, `UnloadFont()` — Font management (used with explicit codepoint vectors for non-ASCII glyphs)
- `SetConfigFlags()` — Must set `FLAG_WINDOW_HIGHDPI` before `InitWindow()` for physical-resolution framebuffer on HiDPI displays
- `GetWindowScaleDPI()` — Query monitor content scale after `InitWindow()`; use to scale font atlas sizes
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

## Crisp Font Rendering — Directives

> Raylib renders fonts via stb_truetype (no FreeType, no subpixel/LCD rendering).
> Glyph coverage is stored in the alpha channel of a GRAY_ALPHA atlas and alpha-blended.
> **There are only two filter modes**: `TEXTURE_FILTER_POINT` (crisp at 1:1, jagged otherwise)
> and `TEXTURE_FILTER_BILINEAR` (smooth always, but blurry).
>
> For the **crispest possible text**, follow all six directives below.

### Directive 1 — Atlas pixel size must match rendered pixel size

When a font atlas is sampled at a size different from its `baseSize`, the mismatch
forces either jagged edges (POINT filter) or blur (BILINEAR filter). The only way to
get pixel-sharp text is to ensure `fontSize == font.baseSize` at draw time.

```cpp
// GOOD — atlas at 28px, rendered at 28px → crisp
Font f = LoadFontEx("path.ttf", 28, codepoints, count);
DrawTextEx(f, text, pos, 28, 1, color);   // 28 / 28 = 1.0 scale → sharp

// BAD — atlas at 24px, rendered at 28px → jagged with POINT, blurry with BILINEAR
Font f = LoadFontEx("path.ttf", 24, codepoints, count);
DrawTextEx(f, text, pos, 28, 1, color);   // 28 / 24 = 1.167 scale → mismatch
```

### Directive 2 — Load one font atlas per distinct rendered size

Every unique `fontSize` gets its own `Font` loaded at that exact pixel size.
Do not reuse a single atlas for multiple sizes.

| Use | Load at | Rendered size | Ratio |
|-----|---------|--------------|-------|
| Body text | `currentFontSize × dpiScale` (e.g. 24px) | Same | 1.0 |
| Section heading | `currentFontSize × 1.3 × dpiScale` | Same | 1.0 |
| Verse number | `currentFontSize × 0.65 × dpiScale` | Same | 1.0 |

Currently the app loads **bodyFont** and **headingFont** at the correct sizes.
If a new distinct rendering size is added (e.g. verse numbers at 65%), a
separate `Font` must be loaded at that size.

### Directive 3 — Always use `TEXTURE_FILTER_POINT`

Point filtering gives zero interpolation between texels, producing the sharpest
results when Directives 1 and 2 are followed. Bilinear filtering introduces
blurriness (raylib's own source at `rcore.c:651` calls it "blurry").

```cpp
SetTextureFilter(font.texture, TEXTURE_FILTER_POINT);
```

### Directive 4 — Reload font atlases when the user changes font size

When the user adjusts font size (12–36px slider), all font atlases must be
unloaded and reloaded at the new pixel size. Simply changing the `fontSize`
parameter to `DrawTextEx` without reloading the atlas violates Directive 1.

Implementation in `App::ReloadFonts()`:
1. `LoadFontEx` new fonts at the new size into temporary `Font` objects
2. `SetTextureFilter(..., TEXTURE_FILTER_POINT)` on both
3. `UnloadFont` the old fonts
4. Copy-assign the temporaries into `bodyFont_` and `headingFont_`
5. Invalidate all cached layouts (forces re-layout with new glyph metrics)

Because all consumers store `const Font&` references to `App::bodyFont_`
and `App::headingFont_`, step 4 automatically propagates the new fonts.

Codepoints are extracted once and cached in `App::fontCodepoints_` to avoid
re-parsing the TTF `cmap` table on every size change.

### Directive 5 — Scale font atlas size by display DPI

On a 2x Retina display, the logical 24px body font must be baked at 48px.
Use `GetWindowScaleDPI()` (raylib, available after `InitWindow`) to query
the monitor content scale:

```cpp
SetConfigFlags(FLAG_WINDOW_HIGHDPI);
InitWindow(450, 800, "TheWord");
Vector2 dpi = GetWindowScaleDPI();
float dpiScale = std::max(dpi.x, dpi.y);
```

Then scale all font loads: `int atlasSize = (int)(logicalSize * dpiScale)`.
On a non-Retina display `dpiScale` is 1.0; on Retina it is 2.0.

### Directive 6 — Enable `FLAG_WINDOW_HIGHDPI`

Without this flag, GLFW creates the framebuffer at the logical window size
(e.g. 450×800) rather than the physical pixel size. On HiDPI displays this
means all rendering (not just text) is at half resolution.

```cpp
SetConfigFlags(FLAG_WINDOW_HIGHDPI);   // must come before InitWindow
InitWindow(config::WINDOW_WIDTH, config::WINDOW_HEIGHT, "TheWord");
```

This also enables raylib's internal screen-scale matrix that maps logical
coordinates to physical framebuffer coordinates.

### Why not...

- **BILINEAR filter?** — Blurry on font textures. Raylib's own code calls it
  "blurry" (`rcore.c:651`). Only acceptable if you can't reload atlases.
- **Subpixel (LCD) rendering?** — Raylib does not support it. No FreeType
  subpixel rendering, no ClearType. Fonts are alpha-blended grayscale.
- **SDF fonts?** — Not used. Would require switching to raylib's SDF pipeline
  and different shaders.
- **Mipmaps for fonts?** — Font atlases are not mipmapped. `GenTextureMipmaps`
  is never called for font textures. Mipmaps only help downscaling, not the
  1:1 case targeted here.

## Font Atlas Loading — Current Implementation

The app loads **two font atlases** at DPI-scaled sizes and reloads them on
font size changes:

1. **bodyFont** — at `(int)(currentFontSize × dpiScale)` — verse text, poetry, verse numbers
2. **headingFont** — at `(int)(currentFontSize × 1.3 × dpiScale)` — section headings,
   chapter labels, book titles, UI headers

Both use `TEXTURE_FILTER_POINT`. Codepoints are extracted from the TTF `cmap`
table by `LoadFontCodepoints()` (see `src/core/FontHelper.cpp`).

## Android Text Input — Raylib Patch

> **Status**: Active | `cmake/patches/raylib-5.0-android-char-input.patch`

Raylib 5.0's Android backend (`rcore_android.c`) has two unfixed issues that break `GetCharPressed()`:

### Issue 1: Gamepad Source Flag Collision

Raylib issue [#5387](https://github.com/raysan5/raylib/issues/5387). On some devices (Motorola Razr 2024, Android 14+), keyboard input events have **both** `AINPUT_SOURCE_KEYBOARD` and `AINPUT_SOURCE_GAMEPAD` bits set (they share `AINPUT_SOURCE_CLASS_BUTTON`). The gamepad handler at line 1000 matches first and returns early, so keyboard events never reach the keyboard handler.

**Fix** (upstream PR [#5441](https://github.com/raysan5/raylib/pull/5439)): add `&& !(source & AINPUT_SOURCE_KEYBOARD)` to the gamepad condition.

### Issue 2: `charPressedQueue` Never Populated

Every other raylib platform populates `charPressedQueue` via a platform-specific callback:
- **Desktop** (GLFW): `CharCallback` — receives UTF-32 codepoints from the OS
- **Web**: JavaScript `keypress` / `input` events
- **DRM**: `EvkeyToUnicodeLUT[]` lookup table

Android has **none of these**. `charPressedQueue` is reset to 0 every frame in `PollInputEvents()` but never filled. `GetCharPressed()` always returns 0.

**Fix**: Inside the `ACTION_DOWN` handler, use JNI to construct a Java `KeyEvent` and call `getUnicodeChar(metaState)`. This returns the Unicode codepoint for the pressed key, accounting for the active keyboard layout and modifier keys (shift, caps lock, etc.). Non-printable keys (function keys, arrows) return 0 and are skipped — matching the behaviour of GLFW's `CharCallback` on desktop. No explicit "text input enabled" gate exists in raylib 5.0.

### Patch Location

The combined patch lives at `cmake/patches/raylib-5.0-android-char-input.patch` and is applied automatically during CMake configuration for Android builds (see `CMakeLists.txt`). It is NOT upstream in raylib 5.0.

### Limitations

- Complex IME composition (CJK, emoji sequences via `ACTION_MULTIPLE` + `AKEYCODE_UNKNOWN`) is not handled. For full IME support, a hidden `EditText` + `TextWatcher` on the Java side would be required (well-known NDK pattern used by SDL2, Unity).
- Only active when `SetTextInputEnabled(true)` has been called, so normal key handling is unaffected.

## Important Note

The `find_package(CURL)` call must come AFTER `FetchContent_MakeAvailable(raylib)` in CMakeLists.txt. If placed before, CMake may fail to find resolved dependencies.
