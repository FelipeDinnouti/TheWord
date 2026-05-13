
> Version 1.0 | Status: Planning

---

## 1. Project Overview

**TheWord** is a minimalist Bible study application built on Raylib. The goal is a simple, distraction-free tool for reading and highlighting Bible text — nothing more. It serves as a lightweight text engine with a clean UI, designed for extensibility.

### Key Characteristics

- **Minimalist**: No bloat — just reading, highlighting, and navigation.
- **Text-centric**: A custom text rendering engine is the core, not an afterthought.
- **Cross-platform**: Desktop proof-of-concept first, Android as primary mobile target.
- **Extensible**: Architecture supports future features like search, notes, and cross-references.

### Decisions

| Dimension | Choice |
|-----------|--------|
| Text Source | USFM (Unfolded Scripture) format |
| Persistence | SQLite for user data |
| Architecture | Monolithic binary with modular code organization |
| Mobile Target | Android (primary) |
| MVP Scope | All Bible books, dynamic infinite scroll, one highlight color |

### Current State

The repository contains a minimal Raylib skeleton with placeholder code and sample JSON data files in Portuguese. No actual features are implemented beyond a working build system.

---

## 2. Architecture Overview

The system is organized into four conceptual layers, each building on the one below it. The layers are implemented as distinct modules within the `src/` directory, allowing clear separation of concerns while keeping the final binary monolithic.

> **See also:**
> - [SPEC.md](Project%20Specification.md) — Project specification with core structures and design goals
> - [Document.md](Document.md) — Document manager design notes covering infinite scroll and anchor-fixed behavior

### 2.1 The Four Layers

> **See also:**
> - [ai-docs/TextRendererReference.md](TextRenderer.md) — Detailed design walkthrough for the text layout engine, including tokenization, word wrapping, and infinite scroll mechanics

The **Data Layer** sits at the foundation. It handles the raw inputs: parsing USFM Bible files into structured data, managing fonts, and persisting user data to SQLite. Nothing above this layer knows anything about file formats or database queries.

The **Text Layout Engine** is where text becomes geometry. It takes raw verse text and transforms it into positioned lines and spans of text with document-space coordinates. This is the most intellectually dense part of the system — it handles tokenization, word wrapping, and the generation of layout metadata that every other layer depends on.

The **Document Manager** uses the layout engine to build a scrollable document. It tracks which chapters are loaded, manages the infinite scroll window, and ensures that prepend operations keep the visible content anchored in place. It knows about document space coordinates, chapter boundaries, and scroll position.

The **UI Layer** sits at the top. It reads input, translates screen coordinates into document positions, queries the highlighting system, and drives the Raylib render loop. Everything the user sees passes through this layer.

### 2.2 Core Data Structures

The data structures span across modules, but several are fundamental to how the system works.

A **Word** is the smallest unit of text. Each word carries a unique identifier used for highlighting, and a reference to its parent verse. The identifier is global across the entire document — every word in Genesis 1 has a lower ID than every word in Genesis 2, for example. This flat numbering scheme makes range-based highlighting straightforward.

```cpp
struct Word {
    int id;              // Global word index
    int verseId;         // Parent verse reference
    std::string text;    // The actual text
};
```

A **Span** is a contiguous run of text with shared properties. It stores the text itself, its position and size in document-space coordinates, and indices into the word array. This last field is what makes highlighting work — a highlight region is defined as a range of word IDs, and the renderer finds all spans that contain words within that range.

```cpp
struct Span {
    std::string text;
    float x, y;          // Document-space position
    float width, height;
    int verseId;         // Verse this span belongs to
    int startWord;       // First word index in this span
    int endWord;         // Last word index in this span
};
```

A **Line** groups spans that share the same baseline. The line height is uniform across a chapter (or document), determined by the font size and line spacing. Each line records its vertical offset from the document origin, and a vector of spans that make up the line.

```cpp
struct Line {
    float y;             // Document-space Y position
    float height;       // Line height (fontSize * lineSpacing)
    std::vector<Span> spans;
};
```

A **ChapterLayout** represents a fully laid-out chapter. It contains the chapter identifier, the document-space start position, the total rendered height, all lines, and a reverse index mapping verse IDs to the spans that belong to them. The reverse index is a performance optimization — without it, finding all spans for a verse would require scanning every span in the chapter.

```cpp
struct ChapterLayout {
    std::string chapterId;           // e.g., "Genesis.1"
    float startY;                    // Document-space start position
    float totalHeight;                // Total rendered height
    std::vector<Line> lines;
    std::unordered_map<int, std::vector<int>> verseToSpans;
};
```

A **Highlight** defines a selected region of text. The region is expressed as a range of word IDs, not as pixel coordinates — this makes it resolution-independent. When the document is re-laid out (e.g., after a font size change), the same word IDs produce the same highlight rectangles.

```cpp
struct Highlight {
    int id;              // Unique highlight ID
    int startWord;       // First word in the region
    int endWord;         // Last word in the region
    int typeId;          // HighlightType reference (color)
};
```

A **HighlightType** represents a named color, such as the default yellow highlight. Multiple types will be supported in the future, but the MVP uses only one.

```cpp
struct HighlightType {
    int id;
    std::string name;
    Color color;
};
```

### 2.3 Module Responsibilities

Each module owns a specific part of the system. The public interface of each module is small and focused, which makes the codebase easier to navigate.

**TextEngine** (in `src/text/`) handles tokenization of USFM text, word wrapping using `MeasureTextEx`, and the generation of spans with document-space coordinates. Its main output is a `ChapterLayout`. It caches layouts so that repeated layout operations are instant.

> **See also:**
> - [ai-docs/TextRendererReference.md](TextRenderer.md) — Detailed layout engine design with code examples
> - [SPEC.md](Project%20Specification.md) — Core data structures (`Word`, `Span`, `Line`)

**DocumentManager** (in `src/document/`) is responsible for infinite scroll. It tracks which chapters are currently loaded, maintains chapter boundaries in document space, and implements the anchor-fixed prepend behavior. It exposes methods for appending and prepending chapters as the user scrolls.

**Highlighter** (in `src/highlight/`) owns the highlight data structures and provides methods for creating, removing, and querying highlights. It depends on the layout engine to map word ranges to pixel rectangles, and on the persistence manager to save and load highlights.

**USFMParser** (in `src/data/`) reads USFM files and produces a tree of Book → Chapter → Verse → Word structures. It is a pure data transformation module with no knowledge of rendering or scrolling.

> **See also:**
> - [SPEC.md](Project%20Specification.md) — Core structures (`Book`, `Chapter`, `Verse`)
> - [ai-docs/TextRendererReference.md](TextRenderer.md) — USFM tokenization approach

**PersistenceManager** (in `src/persistence/`) wraps SQLite operations. It creates the database on first run, manages the schema, and provides CRUD operations for highlights and preferences. It is the only module that knows about SQL.

**InputHandler** (in `src/input/`) translates Raylib input events into document actions. It handles mouse drag selection and touch gestures. It depends on the layout engine to perform hit detection — given a screen position, it asks the engine which word is at that position.

> **See also:**
> - [ai-docs/TextRendererReference.md](TextRenderer.md) — Hit detection and selection model
> - [ai-docs/WINDOWS_VS_LINUX.md](./ai-docs/WINDOWS_VS_LINUX.md) — Platform-specific input handling considerations

**Renderer** (in `src/renderer/`) is the top-level render coordinator. It draws the visible portion of the document by querying the layout engine, draws highlight rectangles by querying the highlighter, and draws UI elements like the chapter title and font size controls.

### 2.4 Coordinate Spaces

> **See also:**
> - [Document.md](Document.md) — Anchor-fixed behavior and scroll position details
> - [ai-docs/TextRendererReference.md](TextRenderer.md) — Document vs. screen coordinate conversion

The system operates in two coordinate spaces. Understanding the distinction is essential for working with any part of the rendering or input handling code.

**Document space** is the coordinate system of the entire document, where (0, 0) is the top-left corner of the first verse of Genesis 1. Every chapter has a start position and a height in document space, and these accumulate as chapters are added. The scroll position is stored in document space.

**Screen space** is the coordinate system of the window, where (0, 0) is the top-left corner of the drawable area. To convert from document space to screen space, subtract the current scroll position from the Y coordinate. To convert from screen space to document space (for hit detection), add the scroll position to the screen Y.

The anchor-fixed behavior relies on this distinction. When the user scrolls to the top of the screen and the system prepends a chapter, the scroll position is increased by the height of the prepended chapter. This shifts all existing content downward in document space, but since the scroll position has been adjusted by exactly the same amount, the visible content stays at the same screen position.

---

## 3. Development Phases

> **See also:**
> - [SPEC.md](Project%20Specification.md) — Project specification and core structures
> - [ai-docs/TextRendererReference.md](TextRenderer.md) — Detailed implementation guidance for text rendering systems

The work is organized into eight sequential phases. Each phase builds on the previous one, and each has clear acceptance criteria that must be met before moving on.

### Phase 1: Project Foundation

> **See also:**
> - [ai-docs/configuring_environment.md](./ai-docs/configuring_environment.md) — CMake setup for Raylib on different platforms
> - [ai-docs/WINDOWS_VS_LINUX.md](./ai-docs/WINDOWS_VS_LINUX.md) — Build system differences between platforms

The first phase establishes a working build system and a modular directory structure. The goal is to have a clean, compilable codebase that follows the architecture decisions outlined above.

The existing CMakeLists.txt already configures Raylib via FetchContent, so the build system foundation is in place. The main tasks are to create the directory structure under `src/`, add a logging utility, and configure the window with a mobile aspect ratio.

The directory structure should separate concerns clearly. The `core/` directory holds cross-cutting utilities like configuration and logging. The `data/` directory holds USFM parsing and data structures. The `text/` directory holds the layout engine. The `document/` directory holds the document manager. The `highlight/` directory holds the highlighting system. The `persistence/` directory holds SQLite operations. The `renderer/` directory holds rendering and UI code. The `input/` directory holds input handling.

This phase should produce a running application that opens a window at the correct aspect ratio with a basic render loop active. No text needs to be rendered yet.

### Phase 2: Text Layout Engine

> **See also:**
> - [ai-docs/TextRendererReference.md](./ai-docs/TextRendererReference.md) — Detailed layout algorithm walkthrough
> - [SPEC.md](Project%20Specification.md) — Core structures: `Word`, `Span`, `Line`, `ChapterLayout`

**Status: Completed**

The text layout engine is the heart of the system. This phase implements robust text tokenization, word wrapping, and span generation.

The engine receives raw verse text and a maximum width, and it produces a complete `ChapterLayout`. The process begins with tokenization: the input string is scanned for verse markers (the `\v` marker in USFM), and the text is split into tokens, each associated with a verse number.

After tokenization, the engine performs word wrapping. Each token's words are appended to the current line until adding the next word would exceed the maximum width. The engine measures each word using Raylib's `MeasureTextEx`. A special rule applies to verse numbers: the verse number and the first word are kept together on the same line. If they do not fit together, both move to the next line. This prevents orphaned verse numbers from appearing at the end of a line.

As words are placed on lines, the engine generates spans. Each span covers a contiguous range of words that belong to the same verse. The span records the document-space position (x and y), the width, the verse ID, and the word range indices. The y position of a span is inherited from the line's y offset.

The engine caches `ChapterLayout` objects keyed by chapter ID. When the window is resized, all cached layouts must be invalidated and rebuilt, because the maximum width has changed.

This phase also includes unit tests for tokenization and word wrapping. The tests verify that verse markers are detected correctly, that words wrap at the correct width, and that verse numbers stay attached to their first words.

**Implementation Details:**
- `src/text/LayoutEngine.h/cpp` — Core layout engine with tokenization and word wrapping
- `src/core/APIClient.h/cpp` — HTTP client for YouVersion API using libcurl
- `src/core/EnvLoader.h/cpp` — `.env` file parser for API key management
- `src/data/BibleClient.h/cpp` — Bible API client with passage fetching
- Default Bible version: BSB (id 3034)

### Phase 3: Document Manager and Infinite Scroll

> **See also:**
> - [Document.md](Document.md) — Document design notes covering infinite scroll
> - [ai-docs/TextRendererReference.md](TextRenderer.md) — Anchor-fixed prepend/append mechanics

With a working layout engine, the next step is to build the infinite scroll mechanism on top of it.

The document manager maintains a list of loaded chapters, each represented as a `ChapterLayout`. It tracks chapter boundaries in document space — the start position and height of each chapter — which allows it to determine which chapters are currently visible and which need to be loaded.

When the user scrolls downward, the document manager monitors the viewport's bottom edge. When it approaches the end of the last loaded chapter, it appends the next chapter. The next chapter's start position is set to the previous chapter's start position plus its height. No scroll adjustment is needed for appending.

When the user scrolls upward and the viewport approaches the start of the first loaded chapter, the document manager prepends the previous chapter. It obtains the new chapter's layout, gets its height, and then adjusts the scroll position by adding that height. This is the anchor-fixed behavior: the content that was visible before the prepend stays at the same screen position after.

The document manager also handles smooth scrolling. Instead of jumping directly to the target scroll position, it lerps toward it each frame. This produces a smooth, responsive scroll feel.

To save memory, the document manager can unload chapters that are far from the viewport. A chapter that is more than a few screen heights away from the current viewport can be discarded, with the caveat that unloaded chapters must be re-laid out if the user scrolls back to them.

This phase produces a document that scrolls smoothly in both directions, with chapters loading and unloading dynamically, and with no visible jumps when prepending content.

### Phase 4: Highlighting System

Highlighting is the core motivation for the entire project. This phase implements per-word hit detection, highlight creation, and highlight rendering.

The highlighter owns a collection of `Highlight` objects. It provides methods to create a highlight given a start and end word ID, to remove a highlight, and to query which highlights cover a given word.

Hit detection is performed by the layout engine. Given a screen position, the engine converts it to document space using the current scroll position, then searches the visible spans to find which one contains that position. Since spans store the range of word IDs they contain, the engine can return the word at a given position by performing a binary search within the matching span.

Drag selection works as follows: when the user presses the mouse (or touches the screen), the input handler calls `getWordAtPosition` to find the word under the cursor and records it as the selection start. As the user drags, the same function is called for the current cursor position to get the selection end. When the user releases, the highlighter creates a new highlight covering the range from start to end.

Rendering highlights is straightforward. For each visible span, the renderer queries the highlighter to find which highlights cover the span's word range. For each covering highlight, it draws a filled rectangle using the highlight's color behind the text. The text is then drawn on top of the rectangle.

The highlighter also coordinates with the persistence manager to save new highlights and load existing ones on startup.

### Phase 5: Data Layer — USFM Parser

> **See also:**
> - [SPEC.md](Project%20Specification.md) — Core data structures (`Book`, `Chapter`, `Verse`, `Word`)
> - [ai-docs/TextRendererReference.md](TextRenderer.md) — USFM tokenization approach

The USFM parser reads USFM Bible files and produces structured data. USFM is a plain-text format with markers like `\c` for chapter, `\v` for verse, and `\p` for paragraph.

The parser tokenizes the input character by character, recognizing marker patterns and building a tree of structures. The top level is a `Book`, which contains a vector of `Chapter` objects. Each `Chapter` contains a vector of `Verse` objects. Each `Verse` contains a vector of `Word` objects.

The parser handles the most common markers (`\c`, `\v`, `\p`) and ignores less common ones (cross-references, footers) for the MVP. These can be progressively added as the system matures.

The document manager uses the parser to obtain chapter text on demand. When a chapter needs to be laid out, the manager asks the parser for the raw text of that chapter, and passes it to the layout engine.

### Phase 6: Data Layer — SQLite Persistence

> **See also:**
> - [SPEC.md](Project%20Specification.md) — Core structures (`Highlight`, `HighlightType`)
> - [ai-docs/WINDOWS_VS_LINUX.md](./ai-docs/WINDOWS_VS_LINUX.md) — Platform-specific SQLite considerations

Highlights must persist between sessions. This phase implements a SQLite-backed persistence layer.

The database schema has three tables. The `highlight_types` table stores named color definitions. The `highlights` table stores highlight regions as word ID ranges, referencing a highlight type. The `preferences` table stores key-value pairs for user settings like font size and last scroll position.

```sql
CREATE TABLE highlight_types (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL,
    color_r INTEGER NOT NULL,
    color_g INTEGER NOT NULL,
    color_b INTEGER NOT NULL
);

CREATE TABLE highlights (
    id INTEGER PRIMARY KEY,
    start_word INTEGER NOT NULL,
    end_word INTEGER NOT NULL,
    type_id INTEGER REFERENCES highlight_types(id)
);

CREATE TABLE preferences (
    key TEXT PRIMARY KEY,
    value TEXT NOT NULL
);
```

The persistence manager wraps all SQLite operations. On first run, it creates the database file and initializes the schema with a default highlight type (yellow). On subsequent runs, it loads existing highlights into memory so they can be displayed immediately.

The high integration point between persistence and the rest of the system is the word ID. Highlights store ranges of word IDs, which are stable across sessions and across re-layouts. This means that if the user changes the font size, the same word IDs are used, and the highlights remain in the correct positions.

### Phase 7: UI Layer and Rendering

> **See also:**
> - [ai-docs/TextRendererReference.md](TextRenderer.md) — Rendering loop and highlight drawing
> - [SPEC.md](Project%20Specification.md) — Project specification and design goals

With all the layers below in place, this phase brings everything together with a clean user interface.

The UI is intentionally minimal. The main reading area takes up the full screen. A subtle top bar shows the current book and chapter name. A bottom bar (or edge gesture) provides navigation and font size controls.

Input handling connects to the document manager and highlighter. Mouse wheel or touch swipe scrolls the document. Tap or drag on text creates highlights. Font size changes trigger a re-layout, and the scroll position is adjusted to keep the anchor verse at the same screen position.

The window resize handler is critical. When the window width changes, all cached layouts become invalid and must be rebuilt. The scroll position must be adjusted to keep the visible content anchored.

An optional debug overlay can display FPS, scroll position, and word count during development. This overlay should be hidden in release builds.

### Phase 8: Mobile Preparation (Android)

> **See also:**
> - [ai-docs/WINDOWS_VS_LINUX.md](./ai-docs/WINDOWS_VS_LINUX.md) — Platform-specific build configuration
> - [ai-docs/configuring_environment.md](./ai-docs/configuring_environment.md) — Build system setup

With a working desktop application, the final phase prepares the codebase for Android.

The main challenges are the build system and input handling. Android requires a CMake toolchain file that targets the Android NDK. Raylib supports Android, but the build configuration is different from the desktop build.

Touch input is the primary interaction method on mobile. The input handler must recognize swipe gestures for scrolling, tap for hit detection, and drag for selection. Pinch-to-zoom for font size is a desirable feature.

Android lifecycle management is also necessary: the app must handle pause and resume correctly, saving and restoring the scroll position and highlight state.

As an intermediate step before Android testing, WebAssembly via Emscripten can be used to test the touch-based interaction model on the desktop browser. This requires a different build configuration but uses the same codebase.

---

## 4. File Structure

> **See also:**
> - [ai-docs/WINDOWS_VS_LINUX.md](./ai-docs/WINDOWS_VS_LINUX.md) — Build system differences between Linux and Windows
> - [ai-docs/configuring_environment.md](./ai-docs/configuring_environment.md) — Platform-specific build configuration

```
TheWord/
├── CMakeLists.txt              # Build configuration
├── README.md                   # Project description
├── LICENSE                     # License file
├── the-word-docs/              # Documentation
│   ├── SPEC.md                 # Project specification
│   ├── Document.md             # Document design notes
│   └── ai-docs/                # AI-assisted design docs
│       ├── TextRendererReference.md
│       ├── configuring_environment.md
│       └── WINDOWS_VS_LINUX.md
├── data/                       # Bible text data
│   ├── schema.sql              # SQLite schema
│   └── [USFM files]           # Bible text in USFM format
├── include/                    # Local header files
├── libs/                       # Third-party headers
├── shaders/                    # Raylib shaders
├── src/
│   ├── main.cpp                # Application entry point
│   ├── core/                   # Cross-cutting utilities
│   │   ├── Config.h            # Application configuration
│   │   └── Logger.h            # Logging utilities
│   ├── data/                   # Data layer
│   │   ├── USFMParser.h/cpp     # USFM file parser
│   │   ├── Book.h/cpp           # Book data structure
│   │   ├── Chapter.h/cpp        # Chapter data structure
│   │   └── Verse.h/cpp         # Verse data structure
│   ├── document/               # Document management
│   │   ├── DocumentManager.h/cpp
│   │   └── ChapterLayout.h/cpp
│   ├── highlight/               # Highlighting system
│   │   ├── Highlighter.h/cpp
│   │   └── HighlightType.h/cpp
│   ├── input/                   # Input handling
│   │   └── InputHandler.h/cpp
│   ├── persistence/             # Persistence layer
│   │   ├── PersistenceManager.h/cpp
│   │   └── schema.sql
│   ├── renderer/                # Rendering and UI
│   │   ├── Renderer.h/cpp
│   │   └── UIManager.h/cpp
│   └── text/                    # Text layout engine
│       ├── LayoutEngine.h/cpp
│       ├── Line.h/cpp
│       ├── Span.h/cpp
│       └── Token.h/cpp
├── tests/                       # Unit tests
│   └── test_layout.cpp
└── android/                     # Android-specific files (Phase 8)
    └── app/
```

---

## 5. Module Dependency Graph

The dependency structure is acyclic and strictly layered. The `main.cpp` entry point depends on the renderer. The renderer depends on the document manager, input handler, and highlighter. The document manager depends on the layout engine and USFM parser. The highlighter depends on the layout engine and persistence manager. The persistence manager is standalone — nothing depends on data structures it owns except itself.

```
main.cpp
  └── Renderer
        ├── UIManager
        │     └── Config
        ├── InputHandler
        └── DocumentManager
              ├── LayoutEngine
              │     └── Token
              └── USFMParser
                    └── Book → Chapter → Verse → Word

Highlighter
  ├── LayoutEngine
  └── PersistenceManager

PersistenceManager
  └── Config
```

---

## 6. Build and Run Commands

### Linux

```bash
cmake -B build
cmake --build build --parallel
./build/theword
```

### Windows (MSYS2 / MinGW)

```bash
cmake -B build -G "MinGW Makefiles"
cmake --build build
./build/theword.exe
```

### Android

```bash
cd android
./gradlew assembleDebug
adb install app/build/outputs/apk/debug/app-debug.apk
```

---

## 7. Testing Strategy

Unit tests cover the text layout engine, specifically tokenization and word wrapping. These are the functions most prone to subtle bugs and most amenable to automated testing.

Integration tests cover USFM parsing and highlight persistence. The parser is tested against known USFM samples to verify that the structure is correct. Persistence is tested by creating highlights, saving to the database, reloading, and verifying equality.

Manual testing covers the UI, scroll feel, and touch interaction. These are difficult to automate and are best evaluated by running the application on a device or emulator.

---

## 8. Acceptance Criteria by Phase

### Phase 1: Project Foundation

- [ ] `cmake -B build && cmake --build build` succeeds on Linux and Windows
- [ ] Window opens with mobile aspect ratio (9:16 or similar)
- [ ] Basic render loop is active with FPS display

### Phase 2: Text Layout Engine

- [ ] USFM verse markers are correctly tokenized
- [ ] Words wrap at the specified width without mid-word breaks
- [ ] Verse numbers stay attached to their first word
- [ ] All spans have correct document-space coordinates
- [ ] Layout caching works (repeated layout is instant)
- [ ] Unit tests pass for tokenization and word wrapping

### Phase 3: Document Manager and Infinite Scroll

- [ ] Scrolling reveals content above and below
- [ ] Prepending adjusts scroll so visible content stays anchored
- [ ] Chapters load and unload dynamically
- [ ] Smooth scrolling with lerp
- [ ] Memory usage stays bounded

### Phase 4: Highlighting System

- [ ] Tapping a word highlights it
- [ ] Dragging across words extends the highlight
- [ ] Highlight rectangles appear with the correct color behind text
- [ ] Highlights persist across app restarts

### Phase 5: USFM Parser

- [ ] Valid USFM files are parsed completely
- [ ] Chapter and verse references are correct
- [ ] Parser handles missing or malformed markers gracefully
- [ ] Parsing is fast (under 1 second for a full Bible)

### Phase 6: SQLite Persistence

- [ ] Database is created automatically on first run
- [ ] Highlights are saved and loaded correctly
- [ ] Preferences persist across sessions
- [ ] No data loss on unexpected app termination

### Phase 7: UI Layer

- [ ] Text is readable and well-formatted
- [ ] Scrolling is smooth and responsive
- [ ] Highlighting is intuitive (tap or drag)
- [ ] Window resize handles correctly
- [ ] UI is clean and unobtrusive

### Phase 8: Mobile Preparation

- [ ] Project builds for Android
- [ ] Touch scrolling works
- [ ] Touch selection and highlighting works
- [ ] Android lifecycle (pause/resume) is handled

---

## 9. Future Considerations

These features are explicitly out of scope for the MVP but are noted here for future planning. They are ordered by likely priority.

- Multiple highlight colors with user-defined types
- Search functionality across books
- Bookmarks for quick navigation
- Notes and annotations attached to verses
- Cross-reference linking
- Dark mode and theming
- Export highlights to a shareable format
- Cloud synchronization
- iOS support
- Parallel scripture view (side-by-side comparison)
- Audio and text-to-speech integration

---

## 10. Glossary

| Term | Definition |
|------|------------|
| **Anchor-fixed** | When content is prepended above the viewport, the scroll position is adjusted by the height of the prepended content so that the content visible before the prepend remains at the same screen position afterward. |
| **Document space** | The coordinate system of the entire document, where (0, 0) is the top of Genesis 1:1. All chapter positions and the scroll position are expressed in document space. |
| **Screen space** | The coordinate system of the window, where (0, 0) is the top-left corner of the drawable area. Input positions and drawing operations use screen space. |
| **USFM** | Unfolding Scripture Format Marked — a plain-text markup format for encoding Bible text with markers like `\c` (chapter) and `\v` (verse). |
| **Span** | A contiguous run of text with shared properties, including a verse reference and a range of word indices. Spans are the unit of rendering and hit detection. |
| **Verse** | A numbered unit of Bible text, such as John 3:16. The verse is the traditional unit of reference for highlighting. |
| **Layout cache** | The storage of computed `ChapterLayout` objects keyed by chapter ID, so that laying out a chapter multiple times is instant. |

---

## Appendix: Reference Documentation

- [Raylib GitHub](https://github.com/raysan5/raylib)
- [USFM Documentation](https://paratext.org/usfm/)
- [SQLite C/C++ Interface](https://www.sqlite.org/cintro.html)
- [Raygui (optional UI library)](https://github.com/raysan5/raygui)

---

## Appendix: Related Documentation

| Document | Description |
|----------|-------------|
| [SPEC.md](Project%20Specification.md) | Project specification with core structures (`Word`, `Span`, `Line`, `Highlight`) and design goals |
| [Document.md](Document.md) | Document manager design notes covering infinite scroll, anchor-fixed behavior, and scroll position management |
| [ai-docs/TextRendererReference.md](TextRenderer.md) | Detailed text layout engine design walkthrough with code examples, including tokenization, word wrapping, and hit detection |
| [ai-docs/WINDOWS_VS_LINUX.md](./ai-docs/WINDOWS_VS_LINUX.md) | Platform-specific build configuration differences between Linux and Windows for Raylib and CMake |
| [ai-docs/configuring_environment.md](./ai-docs/configuring_environment.md) | Step-by-step environment setup guide for MSYS2 and Linux build configurations |
