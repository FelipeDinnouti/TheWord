# Product Requirements

> Status: Updated 2026-06-26 | MVP complete. Phase 11-13 planned.

## MVP Requirements

### Must Have
- [x] Build system compiles on Linux and Windows
- [x] Window opens at mobile aspect ratio
- [x] Rich text rendering (headings, poetry, paragraphs)
- [x] Text rendered with word wrapping
- [x] Layout caching for performance
- [x] Offline USFM Bible text (Bíblia Livre) — fallback when offline
- [x] Online API Bible text (YouVersion) — primary source when online
- [x] Infinite scroll (bidirectional, chapters load dynamically)
- [x] Per-word highlighting (multiple colors)
- [x] Highlights persist across sessions (SQLite)
- [x] Chapter navigation across all 66 books

### Should Have
- [x] Smooth scrolling with momentum
- [x] Font size configuration (A–/A+ buttons, 12–36 range, persisted)
- [x] Touch input support (scroll, tap, pinch, long-press)
- [x] Window resize handling (scroll-anchored re-layout)
- [x] Bible version switching (USFM/Online toggle, persisted)
- [x] Multiple highlight colors (Yellow, Pink, Green, Blue, Orange)
- [x] Context menu (delete/recolor highlights)
- [x] Android NDK build (APK generation)

### Could Have
- [ ] Search across books
- [ ] Dark mode
- [ ] Cloud sync

### Won't Have (MVP)
- [ ] Notes and annotations
- [ ] Cross-reference linking
- [ ] iOS support

## Phase 11-13 Requirements

### Must Have (Phase 11 — Navigation System)
- [ ] Bottom bar with prev/next chapter buttons + current reference (overlays content, appears on scroll-up)
- [ ] Center menu dialog (tapping book code) with Books, Settings, Highlights, Credits options
- [ ] BookList screen (scrollable list, canonical order, search bar, reusable list component)
- [ ] ChapterGrid screen (5-column grid, numbered buttons for each chapter of selected book)
- [ ] Settings screen (full-screen, replaces modal overlay)
- [ ] Navigation stack (push/pop screens with back button, swipe gesture, Escape key)
- [ ] Keyboard shortcuts: ←/→ for prev/next chapter
- [ ] Top bar removed (chapter ref moved to bottom bar)

### Must Have (Phase 12 — Verse Number Identifiers)
- [ ] Verse number spans inserted at layout time when verseId changes
- [ ] Superscript rendering (smaller font + Y offset) for verse numbers
- [ ] Verse number color: grey
- [ ] Verse number format: digit + dot (e.g., "¹.")
- [ ] First verse of each chapter always numbered

### Must Have (Phase 13 — Highlight Browser)
- [ ] Highlight struct extended with bookId, chapterNum, verseStart, verseEnd fields
- [ ] Schema migration for new highlight reference fields
- [ ] HighlightBrowser screen (full-screen)
- [ ] Color filter swatches at top (select highlight color to browse)
- [ ] Scrollable list of highlights matching selected color
- [ ] Each item shows reference + verse text
- [ ] Tapping an item navigates to the verse in the Reader

## User Stories

1. As a reader, I want to open the app and immediately see Bible text so I can start reading.
2. As a reader, I want to scroll through continuous text so I don't have to click "next chapter".
3. As a student, I want to highlight verses so I can mark important passages.
4. As a student, I want my highlights to be saved so I don't lose them when I close the app.
5. As a reader, I want to navigate to any book and chapter so I can find specific passages.
6. As a reader, I want section headings and poetry formatting so the text is structured and readable.
7. As a reader, I want to change font size for comfortable reading.
8. As a reader, I want to switch between Bible versions.
9. As a mobile user, I want touch gestures to scroll and select text.
10. As a reader, I want one-tap prev/next chapter buttons so I don't have to open a dialog each time.
11. As a reader, I want a scroll-up gesture to reveal navigation controls so I can navigate without losing my place.
12. As a reader, I want to see verse numbers so I can reference specific passages when discussing with others.
13. As a student, I want to browse my highlights by color so I can review all passages I marked with a specific theme.
