# Roadmap

> Status: Phase 11 planned | Last Updated: 2026-06-26

The original 10-phase MVP is complete. The app now enters a new phase of UX and feature development.

## Phase Overview

| Phase | Status | Description |
|-------|--------|-------------|
| 1-10 | ✅ Complete | MVP: build, text engine, data sources, highlights, persistence, UI, mobile |
| 11. Navigation System | 🔲 Planned | Bottom bar, center menu, book list, chapter grid, settings screen, navigation stack |
| 12. Verse Number Identifiers | 🔲 Planned | Layout-time verse spans, superscript rendering |
| 13. Highlight Browser | 🔲 Planned | Find highlights by color, browse all matches, navigate to verse |

## Phase 11 — Navigation System

**Goal:** Replace the single-screen overlay pattern with a navigation stack driven by a bottom bar with prev/next chapter buttons and a center menu for accessing books, settings, highlights, and credits.

### Key Deliverables
- Bottom bar: overlay on scroll-up, ◄ / book-ref / ► layout
- Center menu: Books, Settings, Highlights, Credits options
- BookList screen: canonical-ordered scrollable list with search
- ChapterGrid screen: 5-column chapter number grid
- Settings screen: full-screen version of current settings panel
- Navigation stack: push/pop with back button, swipe gesture, Escape
- Keyboard shortcuts: ←/→ for chapter nav

**See:** `02-architecture/UI Philosophy.md` for full design specification.

## Phase 12 — Verse Number Identifiers

**Goal:** Display grey superscript verse numbers with dots before the first word of each verse, rendered at layout time.

### Key Deliverables
- `SegmentType::VerseNumber` added to data model
- `LayoutEngine::LayoutWords()` detects verse transitions and inserts verse number spans
- `Renderer::DrawSpan()` handles VerseNumber spans with superscript positioning
- Theme constants for verse number color and font scale

**See:** `03-modules/Text Layout Engine.md` for design details.

## Phase 13 — Highlight Browser

**Goal:** Allow users to browse all highlights filtered by color, displayed as a scrollable list with verse references, and navigate to any highlighted passage.

### Key Deliverables
- Highlight struct extended with bookId, chapterNum, verseStart, verseEnd
- Schema migration for new reference fields
- HighlightBrowser screen with color filter and scrollable match list
- Navigation to verse from highlight item tap

**See:** `03-modules/Highlighting System.md` for design details.
