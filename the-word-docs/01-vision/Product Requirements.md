# Product Requirements

> Status: Updated 2026-06-26 | All MVP features implemented

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
- [x] Chapter title display (top bar)
- [x] Font size configuration (A–/A+ buttons, 12–36 range, persisted)
- [x] Touch input support (scroll, tap, pinch, long-press)
- [x] Window resize handling (scroll-anchored re-layout)
- [x] Book/chapter go-to dialog with auto-complete
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
