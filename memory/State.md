# Project State

> Last updated: 2026-07-31
>
> Previous: `memory/archive/2026-07-20_v1.8.0-alpha-active.md`

## Version

- **Base**: `1.8.0-alpha`
- **Latest tag**: `v1.8.0-alpha`

## Build Status

| Platform | Status | Notes |
|----------|--------|-------|
| Linux (desktop) | ✅ Builds + runs | GCC/C++17, raylib 5.0, 0 warnings |
| Android (APK) | ✅ Builds + verified | arm64-v8a, VSYNC, lifecycle, footnotes working |
| Windows (cross) | ✅ Builds | `dist/theword-1.6.3-alpha-windows-x86_64.zip` |
| WASM | ⏳ Not verified | Previously built |

## Tests

- **Total**: 80 test cases, 290 assertions
- **Passing**: 78/80 (286/290 assertions)
- **Failing**: 2 (both locale-related — system runs pt_BR, tests expect English)
- **Run**: `./build/theword_test`

## Current Focus

v1.8.0-alpha "Customization & Navigation" complete (released 2026-07-20).

Next release: **v1.9.0-alpha — Architecture Refactor + Code Quality Assurance** (A2, A4, A5, A9 + fresh audit). Full scope confirmed 2026-07-31 — Roadmap wins over the earlier Architecture-Analysis deferral of A4/A9.2-3.

### Completed

- [x] Doc Fixes (release startup)
- [x] Code Quality Audit (all 8 steps)
- [x] Architectural Analysis (9 findings in `memory/Architecture-Analysis.md`)
- [x] Architectural Remediation: A1, A3, A6, A7, A8
- [x] Fuzzy Finder (Book Selection)
- [x] Bible Version Switcher
- [x] Modular Theme System

### Queued

| Workstream | Status |
|------------|--------|
| v1.9.0-alpha — Architecture Refactor + Code Quality Assurance (A2, A4, A5, A9 + fresh audit) | 🔲 Planned (documented in `memory/Active.md`) |
| v1.10.0-alpha — Animations & UI Polish + Web Deployment (WASM verification + `scripts/serve-web.sh`) | 🔲 Planned (documented in `memory/Active.md`) |

### What Changed (v1.8.0-alpha)

- **Fuzzy Finder**: `FuzzyMatcher` with character-sequence scoring + `StripAccents()` for Portuguese UTF-8. Replaced `StartsWithIgnoreCase` in `BookListScreen`.
- **Bible Version Switcher**: 4 versions (BSB, WEB, ASV, BLV). CompositeProvider simplified. Version selection in SettingsScreen.
- **Modular Theme System**: `ThemeManager` + `ThemePalette` (Light/Dark). 34-color palette injected via `const&` across all draw sites. Theme toggle in SettingsScreen. `dark_mode` preference persisted.

### Known Issues

- **Android bottom bar input corruption** — can get stuck after resume (low-severity, deferred)
- Locale-dependent test failures (pt_BR locale) — low priority
- Complex IME composition (CJK, emoji) — deferred to backlog

### Active Decisions

| Decision | Rationale |
|----------|-----------|
| Code audit before feature work | Solid foundation before building |
| Fuzzy Finder replaces prefix-match | Strictly better UX for book search |
| Curated Bible ID list (4 versions) | Proves feature works; extensible |
| ThemeManager runtime object | Enables future themes + user customization |
| Search Across Books deferred | Online-primary app can't ship offline-only search |
| v1.9 = full Roadmap scope (A2, A4, A5, A9 + fresh audit) | Confirmed 2026-07-31 — Roadmap scope wins over Analysis deferral |
| v1.10 = Animations & UI Polish + Web Deployment | Web version never tested; simple static server suffices |
