# Roadmap

> Status: Release-Based Planning Active | Last Updated: 2026-07-18

The original 13-phase MVP is complete. All future work is organized by **release** (SemVer) rather than phases.

## Current: v1.8.0-alpha

**Theme:** Customization & Navigation — Code Quality Audit, Fuzzy Finder, Bible Version Switcher, Modular Theme System

| Status | Item |
|--------|------|
| ✅ | Code Quality Audit — all 8 steps (input consolidation deferred) |
| ✅ | Architectural Remediation — A1/A3/A6/A7/A8 (5 immediate findings fixed) |
| 🔲 | Fuzzy Finder — replace prefix-match book search with fuzzy scoring |
| 🔲 | Bible Version Switcher — curated list of public-domain versions |
| 🔲 | Modular Theme System — runtime-switchable theme, dark mode |

## Planned: v1.9.0-alpha — Architecture Refactor

**Theme:** Layer enforcement and platform decoupling — findings A2, A4, A5, A9

| Status | Item |
|--------|------|
| 🔲 | A2: Extract Span/Line/ChapterLayout from `data/ChapterProvider.h` to `text/LayoutTypes.h` |
| 🔲 | A4: Introduce DrawContext, eliminate direct raylib calls from all screens |
| 🔲 | A5: Abstract MeasureTextEx behind TextMeasureFn |
| 🔲 | A9: Extract FontManager + simplify selection flow through App |

See `memory/Architecture-Analysis.md` for full details on each finding.

Detailed per-release scope tracked in `Release Plan.md`.

## Completed: v1.7.0-alpha

**Theme:** Reading Experience — Copy Verse, Footnotes, Open Where You Left Off, Immersive Mode

| Status | Item |
|--------|------|
| ✅ | Copy Verse Polish — citation format, Ctrl+C, toast feedback |
| ✅ | Footnote Display — data layer, USFM/HTML parsing, markers, popup, cross-refs |
| ✅ | Open Where You Left Off — scroll save/restore |
| ✅ | Immersive / Clean Mode — hide verse numbers, section headings |

## Completed: v1.6.x

**Theme:** Input Refactor, Radial Menu, Android Lifecycle, VSYNC

### v1.6.1-alpha — Input System Refactor + Bug Fixes (tagged)

| Status | Item |
|--------|------|
| ✅ | Unified FSM — merged `HandlePressFSM` + `HandleTouchPressFSM` |
| ✅ | Semantic callbacks — 7 event callbacks replace 3 lifecycle lambdas |
| ✅ | TapDetector helper — removed ~60 lines of duplication across 6 screens |
| ✅ | Dead event removal (`RightClickEvent`, `ScrollStopEvent`) |
| ✅ | Dialog freeze fix — FSM resets on dialog open |
| ✅ | FSM guard for overlays — no word hits on non-ReaderScreen |
| ✅ | Tap-on-release for all screens |
| ✅ | Selection drag fix — decoupled chapter context in Highlighter |
| ✅ | Radial menu hitbox — 1.8× scale |
| ✅ | Build scripts — Linux, Windows cross-compile, Android APK |

### v1.6.2-alpha — Radial Menu + Android Lifecycle (tagged)

| Status | Item |
|--------|------|
| ✅ | Sector-based radial hit detection — eliminates dead zones |
| ✅ | Highlight recolor fix — `HighlightOverlapping()` |
| ✅ | Android lifecycle — surface loss survival, FSM reset on resume |
| ✅ | Save/restore scroll position across OS kills |
| ✅ | Debug tap overlay |

### v1.6.3-alpha — Android VSYNC + Idle Drain (tagged)

| Status | Item |
|--------|------|
| ✅ | EGL VSYNC logging + raylib patch (uncommented `eglSwapInterval`) |
| ✅ | VSYNC re-enabled after context rebind (surface re-creation path) |
| ✅ | Time-based idle drain — ~5fps cap when idle |
| ✅ | Build, deploy, logcat-verified |

## Past Releases

| Version | Theme | Date |
|---------|-------|------|
| v1.5.0-alpha.2 | UI/UX Polish & Cross-Platform Verification | 2026-07-05 |
| v1.5.0-alpha.1 | UI/UX Polish & Cross-Platform Verification | 2026-07-04 |
| v1.4.2 | MVP Completion — Highlight Browser | 2026-06-30 |

Detailed per-release scope is tracked in `Release Plan.md`.

## MVP Complete (Phases 1-13, Historical)

| Status | Phase | Description |
|--------|-------|-------------|
| ✅ Complete | 1-10 | MVP: build, text engine, data sources, highlights, persistence, UI, mobile |
| ✅ Complete | 11. Navigation System | Bottom bar, center menu, book list, chapter grid, settings screen, navigation stack |
| ✅ Complete | 12. Verse Number Identifiers | Layout-time verse spans, superscript rendering |
| ✅ Complete | 13. Highlight Browser | Full screen with color filter, match list, navigation |

Phase details are preserved in git history and `Development Plan.md` for reference.
