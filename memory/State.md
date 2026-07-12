# Project State

> Last updated: 2026-07-12

## Version

- **Base**: `1.6.1-alpha`
- **Latest tag**: `v1.6.1-alpha`

## Build Status

| Platform | Status | Notes |
|----------|--------|-------|
| Linux (desktop) | ✅ Builds + runs | GCC/C++17, raylib 5.0, 0 warnings |
| Android (APK) | ✅ Builds + verified | `dist/theword-arm64-v8a-v1.6.1-alpha.apk` |
| Windows (cross) | ✅ Builds | `dist/theword-1.6.1-alpha-windows-x86_64.zip` |
| WASM | ⏳ Not verified | Previously built |

## Tests

- **Total**: 76 test cases, 267 assertions
- **Passing**: 74/76 (263/267 assertions)
- **Failing**: 2 (both locale-related — system runs pt_BR, tests expect English)
- **Run**: `./build/theword_test`

## Current Focus

**Radial Menu UX/UI + Android Lifecycle** — v1.6.2.

### What's Next

| Workstream | Status |
|------------|--------|
| Input System Refactor + Bug Fixes | ✅ Complete (v1.6.1-alpha) |
| Radial Menu UX/UI | 🔄 Touch targets, press feedback, animations |
| Android Lifecycle | 🔄 Surface re-creation, input reset, save/restore |

### What Changed (v1.6.1-alpha)

| File | Change |
|------|--------|
| `InputHandler.h/cpp` | Unified FSM, 7 semantic callbacks, 15→~12 flags, `Poll(dt, navStack*)` |
| `App.cpp` | Single `onTap` handler replaces 3 lambdas, accumulator consolidated |
| `Events.h` | Removed dead events (`RightClickEvent`, `ScrollStopEvent`) |
| `DocumentManager.h/cpp` | Removed orphan `ScrollStopEvent` forward decl + emit |
| `TapDetector.h` | **New** — reusable press→drag→release utility |
| 6 screen files (CenterMenu, BookList, ChapterGrid, Highlights, Settings, Credits) | Migrated to TapDetector (-2 fields each) |
| `RadialMenu.cpp` | Hitbox scaled to 1.8× visual radius |
| `Highlighter.h/cpp` | Selection carries its own bookId/chapterNum (drag fix) |
| `App.cpp` | FSM guard — overlays blocked from word hits |

### Problems Solved

| Problem | Status |
|---------|--------|
| P1 (timing mismatch) | ✅ — radial click on release via `onTap` |
| P2 (duplicated FSM) | ✅ — single `RunUnifiedFSM()` |
| P3 (flag proliferation) | ✅ — 15 → ~12, clean |
| P4 (callback split) | ✅ — single `onTap` handler in App |
| P5 (touch-down decision) | ✅ — radial hit-test on release |
| P6 (coordinate mismatch) | ✅ — platform-appropriate per Path |
| P7 (dialog freeze) | ✅ — FSM resets on dialog open |
| P8 (6-screen duplication) | ✅ — `TapDetector` helper |
| P9 (dead events) | ✅ — removed |
| P10 (EventBus vs callbacks) | ✅ — callbacks chosen, documented |
| P11 (Selecting vs LongPress) | ✅ — resolved via unified `LongPress` state |
| P12 (poll ordering) | ✅ — documented in comment |

## Known Issues

- **Android lifecycle crash** — tabbing out kills the app (being fixed in v1.6.2)
- **Android bottom bar input corruption** — can get stuck after resume (being fixed in v1.6.2)
- Locale-dependent test failures (pt_BR locale) — low priority
- Complex IME composition (CJK, emoji) — deferred to backlog

## Active Decisions

| Decision | Rationale |
|----------|-----------|
| Input refactor before v1.6.0 tag | Click system accumulated too many patches |
| Unified FSM replaces dual FSMs | Eliminates duplication and P2-divergence |
| Semantic event callbacks replace lifecycle callbacks | Removes touch-down vs touch-up race (P1) |
| App.cpp becomes single decision-maker | Consolidates accumulator + radial lifecycle (P4) |
| Callbacks over EventBus for gesture transport | Simple, sufficient; EventBus can wrap later |
| Dual roadmap system: `docs/` (high-level) + `memory/` (active) | Separates stable reference from sprint ephemera |
