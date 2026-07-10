# Project State

> Last updated: 2026-07-09

## Version

- **Base**: `1.6.0-alpha.1` (built + tested on desktop and Android)
- **Latest tag**: `v1.5.0-alpha.2` (not yet updated)

## Build Status

| Platform | Status | Notes |
|----------|--------|-------|
| Linux (desktop) | ✅ Builds + runs | GCC/C++17, raylib 5.0, 0 warnings |
| Android (APK) | ⏳ Not verified | Builds with `build-android.sh` |
| WASM | ⏳ Not verified | Previously built |
| Windows (cross) | ⏳ Not verified | Previously built |

## Tests

- **Total**: 72 test cases, 233 assertions
- **Passing**: 70/72 (229/233 assertions)
- **Failing**: 2 (both locale-related — system runs pt_BR, tests expect English)
- **Run**: `./build/theword_test`

## Current Focus

**Input System Refactor** — complete. All 8 steps implemented.
Ready for `v1.6.0` tagging.

### What Changed

| File | Change |
|------|--------|
| `InputHandler.h/cpp` | Unified FSM, 7 semantic callbacks, 15→~12 flags, `Poll(dt, navStack*)` |
| `App.cpp` | Single `onTap` handler replaces 3 lambdas, accumulator consolidated |
| `Events.h` | Removed dead events (`RightClickEvent`, `ScrollStopEvent`) |
| `DocumentManager.h/cpp` | Removed orphan `ScrollStopEvent` forward decl + emit |
| `TapDetector.h` | **New** — reusable press→drag→release utility |
| `CenterMenu.h/cpp` | Migrated to TapDetector (-2 fields) |
| `BookListScreen.h/cpp` | Migrated to TapDetector (-2 fields) |
| `HighlightBrowserScreen.h/cpp` | Migrated to TapDetector (-2 fields) |
| `SettingsScreen.h/cpp` | Migrated to TapDetector (-2 fields) |
| `CreditsOverlay.h/cpp` | Migrated to TapDetector (-2 fields) |
| `ChapterGridScreen.h/cpp` | Migrated to TapDetector (-2 fields) |

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
