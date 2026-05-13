
> Current Status: Phase 2 Complete | Last Updated: May 2026

---

## Current Project Status

The project has completed **Phase 1: Project Foundation** and **Phase 2: Text Layout Engine**. The application can now fetch Bible text from the YouVersion API and render it with proper word wrapping and layout. The remaining phases focus on document management, highlighting, persistence, and mobile support.

### Quick Status Overview

| Phase | Status |
|-------|--------|
| Phase 1: Project Foundation | Complete |
| Phase 2: Text Layout Engine | Complete |
| Phase 3: Document Manager and Infinite Scroll | Not Started |
| Phase 4: Highlighting System | Not Started |
| Phase 5: USFM Parser | Not Started |
| Phase 6: SQLite Persistence | Not Started |
| Phase 7: UI Layer and Rendering | Not Started |
| Phase 8: Mobile Preparation (Android) | Not Started |

---

## Detailed Phase Progress

### Phase 1: Project Foundation — Complete

The first phase established a working build system with CMake and Raylib, along with a modular directory structure under `src/`. The project uses FetchContent to download Raylib automatically, and the window is configured with a mobile-first aspect ratio (450x800 pixels). The core utilities for configuration, HTTP client (libcurl), and environment variable loading are all in place.

**What was built:**
- CMake build system with proper project configuration
- Modular directory structure (`core/`, `data/`, `text/`, `document/`, `highlight/`, `persistence/`, `renderer/`, `input/`)
- Window initialization with mobile aspect ratio
- Core utility modules (Config.h, APIClient.h/cpp, EnvLoader.h/cpp)

### Phase 2: Text Layout Engine — Complete

The text layout engine is the heart of the system, handling tokenization of verse text, word wrapping at specified width, and generation of spans with document-space coordinates. The engine receives raw verse text and produces a complete `ChapterLayout` with properly positioned lines and spans. It also includes layout caching so that repeated layout operations are instant.

**What was built:**
- Tokenization of USFM verse markers (`\v`)
- Word wrapping using Raylib's `MeasureTextEx`
- Verse numbers stay attached to their first word
- Span generation with document-space coordinates
- Layout caching keyed by chapter ID
- Integration with YouVersion API (BibleClient.h/cpp)
- API integration: Default Bible version BSB (id 3034), fallback text for John 3:16 when no API key is available

**Files implemented:**
- `src/text/LayoutEngine.h/cpp` — Core layout engine with tokenization and word wrapping
- `src/core/APIClient.h/cpp` — HTTP client for YouVersion API using libcurl
- `src/core/EnvLoader.h/cpp` — `.env` file parser for API key management
- `src/data/BibleClient.h/cpp` — Bible API client with passage fetching
- `src/core/Config.h` — Application configuration constants

### Phase 3: Document Manager and Infinite Scroll — Not Started

With a working layout engine, the next step is to build the infinite scroll mechanism on top of it. The document manager maintains a list of loaded chapters, each represented as a `ChapterLayout`. It tracks chapter boundaries in document space—the start position and height of each chapter—which allows it to determine which chapters are currently visible and which need to be loaded.

**Key features to implement:**
- Maintain a list of loaded chapters as `ChapterLayout` objects
- Track chapter boundaries in document space (start position and height)
- Append next chapter when viewport approaches the end of the last loaded chapter
- Prepend previous chapter when viewport approaches the start of the first loaded chapter, with scroll position adjustment for anchor-fixed behavior
- Implement smooth scrolling with lerp (linear interpolation)
- Unload distant chapters to save memory

**Acceptance criteria:**
- [ ] Scrolling reveals content above and below
- [ ] Prepending adjusts scroll so visible content stays anchored
- [ ] Chapters load and unload dynamically
- [ ] Smooth scrolling with lerp
- [ ] Memory usage stays bounded

### Phase 4: Highlighting System — Not Started

Highlighting is the core motivation for the entire project. This phase implements per-word hit detection, highlight creation, and highlight rendering.

**Key features to implement:**
- Collection of `Highlight` objects with methods to create, remove, and query highlights
- Hit detection via layout engine (convert screen position to document space, find span containing position, binary search for word)
- Drag selection: record start word on press, update end word during drag, create highlight on release
- Render highlight rectangles behind text using highlight color
- Coordinate with persistence manager to save and load highlights

**Acceptance criteria:**
- [ ] Tapping a word highlights it
- [ ] Dragging across words extends the highlight
- [ ] Highlight rectangles appear with the correct color behind text
- [ ] Highlights persist across app restarts

### Phase 5: USFM Parser — Not Started

The USFM parser reads USFM Bible files and produces structured data. USFM is a plain-text format with markers like `\c` for chapter, `\v` for verse, and `\p` for paragraph.

**Key features to implement:**
- Tokenize input character by character, recognizing marker patterns
- Build tree structure: `Book` → `Chapter` → `Verse` → `Word`
- Handle common markers (`\c`, `\v`, `\p`) and ignore less common ones for MVP
- Document manager uses parser to obtain chapter text on demand

**Acceptance criteria:**
- [ ] Valid USFM files are parsed completely
- [ ] Chapter and verse references are correct
- [ ] Parser handles missing or malformed markers gracefully
- [ ] Parsing is fast (under 1 second for a full Bible)

### Phase 6: SQLite Persistence — Not Started

Highlights must persist between sessions. This phase implements a SQLite-backed persistence layer.

**Key features to implement:**
- Database schema: `highlight_types`, `highlights`, `preferences` tables
- Create database on first run with default highlight type (yellow)
- Load existing highlights into memory on startup
- Save new highlights and preferences
- Ensure no data loss on unexpected app termination

**Acceptance criteria:**
- [ ] Database is created automatically on first run
- [ ] Highlights are saved and loaded correctly
- [ ] Preferences persist across sessions
- [ ] No data loss on unexpected app termination

### Phase 7: UI Layer and Rendering — Not Started

With all the layers below in place, this phase brings everything together with a clean user interface.

**Key features to implement:**
- Minimal UI: full-screen reading area, subtle top bar (book/chapter name), bottom bar or edge gesture for navigation and font controls
- Input handling: mouse wheel/touch swipe for scroll, tap/drag for highlighting
- Font size changes trigger re-layout with anchor-fixed scroll adjustment
- Window resize handler: rebuild cached layouts, adjust scroll to keep visible content anchored
- Optional debug overlay (FPS, scroll position, word count) for development

**Acceptance criteria:**
- [ ] Text is readable and well-formatted
- [ ] Scrolling is smooth and responsive
- [ ] Highlighting is intuitive (tap or drag)
- [ ] Window resize handles correctly
- [ ] UI is clean and unobtrusive

### Phase 8: Mobile Preparation (Android) — Not Started

With a working desktop application, the final phase prepares the codebase for Android.

**Key features to implement:**
- CMake toolchain file targeting Android NDK
- Touch input: swipe for scroll, tap for hit detection, drag for selection
- Pinch-to-zoom for font size (desirable)
- Android lifecycle management: handle pause/resume, save/restore scroll position and highlights
- Optional: WebAssembly via Emscripten for browser testing

**Acceptance criteria:**
- [ ] Project builds for Android
- [ ] Touch scrolling works
- [ ] Touch selection and highlighting works
- [ ] Android lifecycle (pause/resume) is handled

---

## Implementation Notes

### What's Been Done

The text layout engine is fully operational. Bible text can be fetched from the YouVersion API using the BSB version (id 3034), and verses are rendered with proper word wrapping. The layout engine caches `ChapterLayout` objects for performance, and the API client handles HTTP requests via libcurl. Environment variables are loaded from a `.env` file for API key management.

### What's Next

The next immediate step is Phase 3: implementing the document manager and infinite scroll. This builds on the existing layout engine by tracking loaded chapters and their document-space positions. The key challenge is implementing the anchor-fixed prepend behavior—when a chapter is prepended above the viewport, the scroll position must be adjusted so that the previously visible content stays in place.

### Key Technical Details

- **Global word IDs**: Every word in the Bible is numbered sequentially (Genesis 1:1 words have lower IDs than Genesis 1:2 words). This makes range-based highlighting straightforward and stable across re-layouts.
- **Coordinate spaces**: Document space (0,0 = top of Genesis 1:1) vs. screen space (0,0 = window top-left). Convert by adding/subtracting scroll position from Y coordinate.
- **Layout caching**: LayoutEngine caches ChapterLayout objects keyed by chapter ID. Cache must be invalidated on window resize (when max width changes).

---

## See Also

- [DEVELOPMENT_PLAN.md](Development%20Plan.md) — Full development plan with architecture details
- [SPEC.md](Project%20Specification.md) — Project specification and core structures
- [Document.md](Document.md) — Document manager design notes
- [ai-docs/TextRendererReference.md](./ai-docs/TextRendererReference.md) — Detailed layout engine design