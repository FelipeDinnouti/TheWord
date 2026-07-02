# Development Plan

> Status: Phases 1-13 complete. Mobile/Android polish remaining.

## Phase Overview

| Phase | Status | Description |
|-------|--------|-------------|
| 1. Project Foundation | ✅ Complete | Build system, directory structure, window setup |
| 2. Text Layout Engine | ✅ Complete | Tokenization, word wrapping (pre-segment) |
| 3. Document Manager | ✅ Complete | Infinite scroll, chapter lifecycle |
| 4. Architecture Foundation | ✅ Complete | Renderer extraction, ChapterProvider, Segment model |
| 5. USFM Parser | ✅ Complete | Offline Bible data source |
| 6. BibleClient (HTML API) | ✅ Complete | Online data source |
| 7. Highlighting System | ✅ Complete | Per-word selection, highlight rendering |
| 8. SQLite Persistence | ✅ Complete | Save/load highlights and preferences |
| 9. UI Layer | ✅ Complete | Navigation, font controls, smooth scroll, heading differentiation, theme consolidation |
| 10. Mobile Preparation | ✅ Complete | Android NDK multi-ABI, WASM, touch gestures, life cycle |
| 11. Navigation System | ✅ Complete | Bottom bar, center menu, book list, chapter grid, settings screen, navigation stack |
| 12. Verse Number Identifiers | ✅ Complete | Layout-time verse spans, superscript rendering |
| 13. Highlight Browser | ✅ Complete | Full screen with color filter, match list, navigation |

## Post-MVP: Release-Based Planning

Phase-based development was ideal for the MVP because each phase was a clearly bounded technical milestone. Now that the core is complete, development shifts to **release-based planning** using SemVer:

| Concept | What It Means |
|---------|---------------|
| **Release** | A tagged, buildable, tested version distributed to users |
| **Version** | Bumped *only* at release time (`MAJOR.MINOR.PATCH`) |
| **Features** | Picked from a backlog per-release, not a linear phase plan |
| **Bug fixes** | Accumulate during a release cycle, shipped in the next PATCH |

### How It Works

1. The current release's plan lives in `04-planning/Release Plan.md`
2. Each release picks 3-5 items from the backlog + pending bug fixes
3. Feature branches are optional — `main`-only workflow is the default
4. When the release checklist is complete: bump version → build → test → tag → distribute
5. Pre-release versions (`-alpha.1`, `-rc.1`) for testing with friends

### What's Next

The remaining MVP-phase work (Phase 10 Mobile/Android polish) and all future features are now planned per-release in `Release Plan.md`. Tracked in `Progress Tracking.md`.
