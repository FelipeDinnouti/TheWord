# TheWord — Phase 8: SQLite Persistence

## Goal

Swap the in-memory `InMemoryStorage` (Phase 7) for a real `PersistenceManager` backed by SQLite. Highlights survive app restart. No changes to the `Highlighter` class — `PersistenceManager` implements the same `PersistenceInterface` that `InMemoryStorage` already implements.

## Design Decisions

1. **SQLite via bundled amalgamation** — FetchContent downloads `sqlite3.c` + `sqlite3.h` (single-file amalgamation). Zero system prerequisites beyond what's already required, consistent with raylib/doctest dependency strategy.
2. **Database at `~/.theword/highlights.db`** — XDG-standard location, survives rebuilds and `rm -rf build`. The `PersistenceManager` creates `~/.theword/` on first run if it doesn't exist.

## Database Schema

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

On first run: `INSERT INTO highlight_types (id, name, color_r, color_g, color_b) VALUES (1, 'Yellow', 255, 255, 0);`

## File Changes

### New Files

| File | Purpose |
|------|---------|
| `src/persistence/PersistenceManager.h` | Class declaration implementing `PersistenceInterface` |
| `src/persistence/PersistenceManager.cpp` | SQLite CRUD: schema init, load/save/remove highlights, types, preferences |

### Modified Files

| File | Changes |
|------|---------|
| `src/core/Config.h` | Add `DB_DIR` (`.theword`), `DB_FILE` (`highlights.db`) constants |
| `src/main.cpp` | Replace `InMemoryStorage storage` with `PersistenceManager storage("...")` — one or two lines |
| `CMakeLists.txt` | FetchContent for sqlite3 amalgamation; populate `PERSISTENCE_SOURCES`; add include dir |
| `tests/test_main.cpp` | Add 4 tests: save→load round-trip, remove highlight, highlight types, preferences |
| `the-word-docs/03-modules/Persistence.md` | Mark Phase 8 complete |
| `the-word-docs/04-planning/Progress Tracking.md` | Phase 8 → ✅ |

## Execution Order

1. **CMakeLists.txt** — Add FetchContent for sqlite3 amalgamation, populate `PERSISTENCE_SOURCES` with `PersistenceManager.cpp` + `sqlite3.c`
2. **Config.h** — Add `DB_DIR` and `DB_FILE` constants
3. **PersistenceManager.h** — Class with `sqlite3* db`, implements `PersistenceInterface`, plus `loadHighlightTypes()`, `saveHighlightType()`, `getPreference()`, `setPreference()`
4. **PersistenceManager.cpp** — Constructor opens/creates DB, calls `initSchema()`; destructor closes DB; CRUD methods
5. **main.cpp** — Swap `InMemoryStorage` → `PersistenceManager` with `~/.theword/highlights.db` path
6. **tests/test_main.cpp** — 4 tests
7. **Build, run tests, verify** — Existing highlights from Phase 7 vanish (different storage), new highlights persist across restarts
8. **Docs** — Persistence.md, Progress Tracking.md

## Files NOT needing changes

- `Highlighter.h/cpp` — unchanged, already uses `PersistenceInterface&`
- `InMemoryStorage.h/cpp` — kept for unit tests, not removed
- `Renderer.h/cpp` — no rendering changes
- `DocumentManager.h/cpp` — no changes
- `LayoutEngine.h/cpp` — no changes
- `PersistenceInterface.h` — no changes (shared contract is stable)
