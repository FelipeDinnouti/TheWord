# Project Specification

> Version 2.0 | Status: Active Development

## Vision

A simple and minimalist Bible app for configurable reading and highlighting — a tool for studying the Bible, nothing more. Essentially a mini text engine + UI framework, using Raylib as the base to render text and handle window management.

The goal is a mobile app, but the proof-of-concept targets desktop with a mobile window ratio (450×800).

## Core Capabilities

**Rendering:**
- Text layout with word wrapping and line breaking
- Rich text support: section headings, poetry indentation, paragraph spacing
- Word indexing (every word has a global unique ID)
- Highlight rectangles aligned to text
- Infinite scrolling document with a sliding window of loaded chapters

**Interaction:**
- Per-word hit detection
- Drag selection to highlight
- Smooth scroll with momentum

**Persistence:**
- Highlights persist between sessions (SQLite)
- User preferences (font size, last position)

## Design Philosophy

- **Minimalist**: No bloat — just reading, highlighting, and navigation.
- **Text-centric**: A custom text rendering engine is the core, not an afterthought.
- **Cross-platform**: Desktop proof-of-concept first, Android as primary mobile target.
- **Extensible**: Architecture supports future features like search, notes, and cross-references.

## Key Decisions

| Dimension | Choice |
|-----------|--------|
| Text Source | **Dual**: HTML API (online primary) + USFM files (offline fallback) |
| Data Abstraction | `ChapterProvider` interface with two implementations |
| Rich Text Model | `Segment[]` + `Word[]` in `ChapterData` |
| Persistence | SQLite for user data |
| Architecture | Monolithic binary with modular code organization |
| Mobile Target | Android (primary) |
| MVP Scope | All Bible books, dynamic infinite scroll, one highlight color, rich text |
| Window Size | 450×800 (mobile-first aspect ratio) |
| Default Bible | Bíblia Livre (CC BY 4.0, Portuguese) |
