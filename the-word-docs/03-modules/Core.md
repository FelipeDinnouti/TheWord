# Core Module

> Status: Updated 2026-06-22

Files: `src/core/Config.h`, `src/core/APIClient.h/cpp`, `src/core/EnvLoader.h/cpp`, `src/core/GlobalId.h`

## Config (`Config.h`)

Application-wide constants in the `config` namespace.

```cpp
namespace config {
    constexpr int WINDOW_WIDTH = 450;
    constexpr int WINDOW_HEIGHT = 800;
    constexpr int TARGET_FPS = 60;
    constexpr float FONT_SIZE = 24.0f;
    constexpr float FONT_HEADING_SIZE = FONT_SIZE * 1.3f; // 31.2px
    constexpr float LINE_SPACING = 1.2f;
    constexpr const char* USFM_DIR = "assets/usfm";
    constexpr const char* ENV_FILE = ".env";
    constexpr const char* YVP_APP_KEY = "YVP_APP_KEY";
    // Font paths for SourceSerif (regular + bold + italic if available)
    constexpr const char* FONT_REGULAR = "assets/fonts/source_serif_4/SourceSerif4-Regular.ttf";
    constexpr const char* FONT_BOLD = "assets/fonts/source_serif_4/SourceSerif4-Bold.ttf";
}
```

Note: `DEFAULT_BIBLE_ID` and `DEFAULT_VERSE` have been removed. The app now starts at Genesis 1 using the USFM parser.

## APIClient (`APIClient.h/cpp`)

HTTP client wrapper around libcurl. Handles GET requests with custom headers.

**Interface:**
- `APIClient()` — Creates and configures a CURL handle
- `~APIClient()` — Cleans up the CURL handle
- `get(url)` — Performs an HTTP GET, returns response body as string
- `setAppKey(key)` / `getAppKey()` — Manage the X-YVP-App-Key header

**Notes:**
- **Optional dependency**: Only needed when using BibleClient (online source)
- CURL handle is created once in constructor, reused for all requests
- Timeout: 10 seconds
- Follows redirects automatically

## GlobalId (`GlobalId.h`)

Single shared counter for globally unique word IDs. An inline function with a function-local `static` ensures a single counter across all translation units.

```cpp
inline int GetNextWordId() {
    static int id = 0;
    return id++;
}
```

Used by all `ChapterProvider` implementations (`USFMParser`, `BibleClient`, `StubChapterProvider`) to assign unique IDs to every word. This guarantees the contract in `Data Structures.md`: "Every word in the Bible has a globally unique ID" — even when `CompositeProvider` falls back between providers.

## EnvLoader (`EnvLoader.h/cpp`)

Loads environment variables from a `.env` file and system environment.

**Interface:**
- `load(filepath)` — Parse `.env` file, store key-value pairs
- `get(key)` — Return value for key (checks file vars first, then env vars)
- `get(key, defaultValue)` — Return value or fallback

**Notes:**
- **Optional dependency**: Only needed for API key loading
- Format: `KEY=VALUE`, `# comments`
- Supports quotes stripping and whitespace trimming
