# Project State

> Last updated: 2026-08-10
>
> Previous: `memory/archive/2026-07-20_v1.8.0-alpha-active.md`

## Version

- **Base**: `1.9.1-alpha`
- **Latest tag**: `v1.9.1-alpha` (2026-08-10)

## Build Status

| Platform | Status | Notes |
|----------|--------|-------|
| Linux (desktop) | ✅ Builds + runs | GCC/C++17, raylib 5.0, 0 warnings |
| Android (APK) | ✅ Builds + device-verified | arm64-v8a, v1.9.1-alpha, VSYNC, lifecycle, footnotes working |
| Windows (cross) | ⏳ Not rebuilt since v1.6.3 | `dist/theword-1.6.3-alpha-windows-x86_64.zip` |
| WASM | ⏳ Not verified | Previously built |

## Tests

- **Total**: 87 test cases, 320 assertions
- **Passing**: 85/87 (316/320 assertions)
- **Failing**: 2 (both locale-related — system runs pt_BR, tests expect English)
- **Run**: `./build/theword_test`

## Current Focus

v1.9.1-alpha "Full Code Audit" complete and device-verified (2026-08-10).

**v1.9.1-alpha done:** input consolidation (InputFrame snapshot, zero polling
in `src/ui/`), App slimming + `onShortcut`, event governance (`KeyEvent`/
`DialogEvent` deleted, Event Bus.md matrix), dead `HasChapter` interface chain
deleted, dead code sweep (OpenURL/GetClipboard/onDragStart/onDragUpdate/
prevScrollY), 7 new test cases. APK device-verified — no regressions, no visual
alteration.

### v1.9.1-alpha — Full Code Audit ✅ (2026-08-10)

- [x] Input consolidation (Phases 2): `InputFrame` core struct; `BeginFrame/Frame/Poll`
- [x] App slimming (Phase 3): Init split ×5, Highlighter statics, D6 dedup, onShortcut
- [x] Event governance (Phase 4): KeyEvent/DialogEvent deleted, Event Bus.md
- [x] Data-layer (Phase 5): HasChapter chain deleted, tests converted
- [x] Tests (Phase 6): 6 LayoutEngine FakeMeasure cases, Highlighter statics cases
- [x] Clean-code (Phase 7): OpenURL/GetClipboard/onDragStart/onDragUpdate/prevScrollY removed
- [x] Release: version 1.9.1-alpha, APK device-verified, tagged `v1.9.1-alpha` (2026-08-10)

### Completed

- [x] A2: LayoutTypes extraction (`text/LayoutTypes.h`) — renderer→data layer skip fixed
- [x] A5: TextMeasureFn abstraction — text/ layer raylib-free
- [x] A9.1: selection-hop removal — InputHandler → App → Highlighter
- [x] A9.2-3: FontManager extraction — App slimming, Settings font-size bug fixed
- [x] A4: DrawContext — all screens/renderer draw through one context bundle
- [x] Device verification of full refactor (theme, font size, all screens, reader interactions)

### Queued

| Workstream | Status |
|------------|--------|
| v1.10.0-alpha — Animations & UI Polish + Web Deployment (WASM verification + `scripts/serve-web.sh`) | 🔲 Planned (documented in `memory/Active.md`) |

### What Changed (v1.9.0-alpha)

- **A2**: `Span`/`Line`/`ChapterLayout` moved to `text/LayoutTypes.h`; `data/ChapterProvider.h` slimmed; no renderer→data include.
- **A5**: `TextMeasureFn` (`text/TextMeasure.h`) replaces `MeasureTextEx` in LayoutEngine — `text/` no longer includes raylib.
- **A9.1**: `InputHandler` emits `SelectionEvent` directly; App's re-emission hop removed; only Highlighter listens.
- **A9.2-3**: `FontManager` (5 fonts, codepoints, sizes, reload) extracted from App; `FontKind` enum; fixed SettingsScreen font-size setting never applying.
- **A4**: `renderer/DrawContext.h` (ThemeManager + FontManager + UIScale) built fresh each frame in App; `Screen::Draw(DrawContext&)`; ~80 direct raylib draw calls removed; Renderer ctor 9 params → 1; draw-side only (input consolidation deferred to Feature 6).

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
| A4 draw-side only | Input consolidation is a separate concern — Feature 6 |
| Release-alpha approval gate | User proves APK on device before tag/commit — no regressions, no visual alteration |