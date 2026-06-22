# Persistence Module

> Status: Planned (Phase 8) | Last Updated: 2026-06-22

## Overview

SQLite-backed persistence layer for highlights and user preferences.

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

## Default Data

On first run, create a default highlight type (yellow):
```sql
INSERT INTO highlight_types (id, name, color_r, color_g, color_b)
VALUES (1, 'Yellow', 255, 255, 0);
```

## Interface

```cpp
class PersistenceManager {
public:
    PersistenceManager(const std::string& dbPath);

    // Highlights
    std::vector<Highlight> loadHighlights();
    void saveHighlight(const Highlight& h);
    void removeHighlight(int id);

    // Highlight types
    std::vector<HighlightType> loadHighlightTypes();
    void saveHighlightType(const HighlightType& t);

    // Preferences
    std::string getPreference(const std::string& key, const std::string& defaultValue);
    void setPreference(const std::string& key, const std::string& value);
};
```

## Design Constraint: Coupling with Highlighting (Phase 7)

The persistence manager and the highlighter share the same core data. **Do not implement one without considering the other.** See `02-architecture/Architecture Overview.md` → "Cross-Cutting Concerns → 1. Highlight + Persistence" for the full discussion.

The persistence manager should implement a `PersistenceInterface` defined by the highlighting system (`src/highlight/PersistenceInterface.h`). This allows the highlighter to work with an in-memory stub during Phase 7 and swap to the real SQLite implementation in Phase 8 without code changes.

```cpp
class PersistenceManager : public PersistenceInterface {
    // ... implements loadHighlights, saveHighlight, removeHighlight
};
```

## Files

- `src/persistence/PersistenceManager.h/cpp`
- `src/highlight/PersistenceInterface.h` (interface to implement)
- Database file: `~/.theword/highlights.db` or `./data/theword.db` (TBD)
