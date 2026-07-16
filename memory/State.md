# Project State

> Last updated: 2026-07-16
>
> Archive: `memory/archive/2026-07-16_v1.7.0-alpha-complete.md`
>
> Previous: `memory/archive/2026-07-14_v1.6.x-complete.md`

## Version

- **Base**: `1.7.0-alpha`
- **Latest tag**: `v1.6.3`

## Build Status

| Platform | Status | Notes |
|----------|--------|-------|
| Linux (desktop) | ✅ Builds + runs | GCC/C++17, raylib 5.0, 0 warnings |
| Android (APK) | ✅ Builds + verified | arm64-v8a, VSYNC, lifecycle, footnotes working |
| Windows (cross) | ✅ Builds | `dist/theword-1.6.3-alpha-windows-x86_64.zip` |
| WASM | ⏳ Not verified | Previously built |

## Tests

- **Total**: 77 test cases, 279 assertions
- **Passing**: 75/77 (275/279 assertions)
- **Failing**: 2 (both locale-related — system runs pt_BR, tests expect English)
- **Run**: `./build/theword_test`

## Current Focus

v1.7.0-alpha "Reading Experience" is complete. Ready for tag and release.

### Completed: v1.7.0-alpha

| Workstream | Status |
|------------|--------|
| App.cpp Refactor — Run() extraction | ✅ |
| Copy Verse Polish | ✅ |
| Immersive / Clean Mode | ✅ |
| Verse Flow Fix | ✅ |
| Bible ID Fix (129→3034) | ✅ |
| Open Where You Left Off | ✅ |
| Footnote Display (incl. cross-refs) | ✅ |

### What Changed (v1.7.0-alpha)

See archived file: `memory/archive/2026-07-16_v1.7.0-alpha-complete.md`

### What Changed (v1.6.x)

See archived file: `memory/archive/2026-07-14_v1.6.x-complete.md`

## Known Issues

- **Android bottom bar input corruption** — can get stuck after resume (low-severity, deferred)
- Locale-dependent test failures (pt_BR locale) — low priority
- Complex IME composition (CJK, emoji) — deferred to backlog

## Active Decisions

| Decision | Rationale |
|----------|-----------|
| Unified FSM replaces dual FSMs | Eliminates duplication |
| Semantic event callbacks over lifecycle callbacks | Removes touch-down vs touch-up race |
| App.cpp single decision-maker | Consolidates accumulator + radial lifecycle |
| Callbacks over EventBus for gesture transport | Simple, sufficient; EventBus can wrap later |
| Dual roadmap system: `docs/` + `memory/` | Separates stable reference from sprint ephemera |
| USFM footnote two-pass extraction | Pre-extract tracking \c/\v, distribute post-parse | Avoids multi-line issues |
| BibleClient footnote self-contained extraction | Tracks yv-v internally, no caller ref needed |
| `[n]` markers use `startWord=-1` | Prevents misidentified word taps |
| FootnotePopup owned by UIManager | Same overlay pattern as RadialMenu |
