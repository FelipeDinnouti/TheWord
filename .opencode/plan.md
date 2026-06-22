# TheWord — Phase 9: MVP Final UI

## Goal

Polish the app into a complete MVP: highlight management (multiple colors, deletion), navigation (book/chapter picker, version switching, font controls), and visual polish (splash screen, heading differentiation).

## Design Decisions

1. **Tap-and-hold / right-click** on highlighted word shows context menu with trash icon + pastel color swatches
2. **5 pastel colors** seeded in DB: Yellow `#FFEB3B`, Pink `#F48FB1`, Green `#A5D6A7`, Blue `#90CAF9`, Orange `#FFCC80`
3. **Bible version switching** via settings toggle (offline USFM ↔ online API), not per-chapter auto-detect
4. **Splash screen** is text-only: "TheWord" centered + "Loading..." subtitle
5. **Preferences** (font size, active version, active color) persisted to `preferences` table

## Execution Order

### Sprint 1: Foundation

1. **InputHandler class** — Extract mouse/keyboard/wheel from `main.cpp` into `src/input/InputHandler.h/cpp`
2. **UIManager class** — `src/renderer/UIManager.h/cpp` — top bar, settings panel overlays, context menu
3. **Smooth scroll refinements** — `DocumentManager.cpp` ease curve tweaks
4. **Window resize polish** — `main.cpp` improve reflow anchor on resize

### Sprint 2: Highlight UX

5. **Highlight hit-testing** — `Highlighter::highlightAtWord(wordId)` returning `const Highlight*`
6. **Context menu widget** — Popup at word position with trash + 5 color swatches
7. **Multiple highlight colors** — Seed 5 types in DB, `Highlighter::activeTypeId` set by UI, `endSelection()` uses it
8. **Delete highlight** — Wire trash button → `Highlighter::removeHighlight(id)` → `persistence.removeHighlight(id)`

### Sprint 3: Navigation & Settings

9. **Book/chapter navigation** — Go-to dialog in UIManager with book code auto-complete
10. **Font size controls** — +/- buttons, live update LayoutEngine + Renderer, persist to preferences
11. **Bible version switching** — `CompositeProvider::setPrimary(ChapterProvider&)`, settings toggle in UIManager
12. **Preferences persistence** — Read/write font size, active version, active color from `preferences` table

### Sprint 4: Polish

13. **Heading differentiation** — Split `drawSpan` case: BookTitle (1.6× BLACK), SectionHeading (1.3× DARKGRAY), ChapterLabel (1.6× gray)
14. **Splash screen** — "TheWord" + "Loading..." until USFM + DB ready
15. **About/credits overlay** — Modal with version, Bíblia Livre (CC BY 4.0), Raylib, SQLite credits

## File Changes

### New Files

| File | Purpose |
|------|---------|
| `src/input/InputHandler.h/cpp` | Extracted mouse/keyboard/wheel handling |
| `src/renderer/UIManager.h/cpp` | Top bar, settings, context menu, dialogs |

### Modified Files

| File | Changes |
|------|---------|
| `src/main.cpp` | Thin orchestrator — init subsystems, loop, cleanup. Input and UI moved to classes |
| `src/highlight/Highlighter.h/cpp` | Add `highlightAtWord()`, `removeHighlight()`, `activeTypeId` |
| `src/persistence/PersistenceManager.cpp` | Seed 5 highlight types instead of 1 |
| `src/data/CompositeProvider.h/cpp` | Add `setPrimary(ChapterProvider&)` for runtime switching |
| `src/renderer/Renderer.cpp` | Split heading case in `drawSpan` |
| `src/document/DocumentManager.cpp` | Smooth scroll tweaks |
| `CMakeLists.txt` | Add `INPUT_SOURCES` and new files |

### Files NOT needing changes

- `LayoutEngine.h/cpp` — no layout logic changes for heading differentiation
- `PersistenceInterface.h` — stable contract
- `InMemoryStorage.h/cpp` — still used by Phase 7 tests
- `BibleBooks.h` — stable data
- `ChapterProvider.h` — stable interface

## Database Schema Updates

On first run (new installs), seed 5 pastel types:

```sql
INSERT INTO highlight_types (id, name, color_r, color_g, color_b)
VALUES (1, 'Yellow', 255, 235, 59),
       (2, 'Pink', 244, 143, 177),
       (3, 'Green', 165, 214, 167),
       (4, 'Blue', 144, 202, 249),
       (5, 'Orange', 255, 204, 128);
```

Existing databases keep their single Yellow entry — new colors will be inserted with `INSERT OR IGNORE` on next migration.
