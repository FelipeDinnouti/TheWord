# Project State

> Last updated: 2026-07-05

## Version

- **Base**: `1.6.0-alpha.1` (configured, not yet built)
- **Latest tag**: `v1.5.0-alpha.2` (not yet updated)

## Build Status

| Platform | Status | Notes |
|----------|--------|-------|
| Linux (desktop) | ✅ Builds + runs | GCC/C++17, raylib 5.0 |
| Android (APK) | ✅ Builds | x86_64 + arm64-v8a via `build-android.sh` |
| WASM | ✅ Builds | Via Emscripten, `build-wasm/theword.html` |
| Windows (cross) | ✅ Builds | MinGW cross-compile |

## Tests

- **Total**: 72 test cases, 233 assertions
- **Passing**: 70/72 (229/233 assertions)
- **Failing**: 2 (both locale-related — system runs pt_BR, tests expect English)
  - `BibleBooks ChapterIdToTitle` — expects "John 3", "Genesis 1"; gets "João 3", "Gênesis 1"
  - `PersistenceManager highlight types` — expects "Yellow", "Orange"; gets "Amarelo", "Laranja"
- **Run**: `./build/theword_test`

## Current Focus

Implementing **v1.6.0-alpha.1** with two features:
1. Copy Verse — right-click/long-press to copy verse text to clipboard
2. Code quality — `unique_ptr<ContextMenu>` refactor

## Known Issues

- Locale-dependent test failures (pt_BR locale) — low priority, test assertions need `Locale::Get()` awareness
- Chapter grid crash on mobile (intermittent, not yet diagnosed)
- Complex IME composition (CJK, emoji) — deferred to backlog
- Phase 10 items (immersive mode, lifecycle, WASM persistence) — deferred to backlog

## Active Decisions

| Decision | Rationale |
|----------|-----------|
| 1.5.0-stable will never ship | Strictly alpha development; 1.5.0-alpha.2 is the final 1.5.0 pre-release |
| v1.6.0 starts fresh with new features | No reason to release 1.5.0 stable when we're not doing widespread testing |
| Two features per release (feature + code quality) | Keep releases focused but not too small |
