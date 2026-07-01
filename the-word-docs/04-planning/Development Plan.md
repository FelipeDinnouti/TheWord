# Development Plan

> Status: Phase 12 complete, Phase 13 in progress.

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
| 13. Highlight Browser | 🔄 In Progress | Data model/highlighter/persistence done; UI pending |

## What's Next

Phase 13 Highlight Browser is the current focus. Steps 1-3 (data model, highlighter, persistence) are complete. Remaining work is tracked in `Progress Tracking.md`.
