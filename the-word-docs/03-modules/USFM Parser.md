# USFM Parser

> Status: Updated 2026-06-22 | Priority: Phase 5 (immediate)

Files: `src/data/USFMParser.h/cpp`

## Overview

The USFM parser is the **offline data source**. It reads USFM Bible files from `assets/usfm/` and produces `ChapterData` with rich-text structure. When the YouVersion API is available, BibleClient is the primary source and USFMParser acts as the fallback.

USFM (Unfolding Scripture Format Marked) is a plain-text markup format for encoding Bible text used by most Bible translation projects.

## Interface

```cpp
class USFMParser : public ChapterProvider {
public:
    explicit USFMParser(const std::string& usfmDir);
    bool HasChapter(const std::string& bookId, int chapter) const override;
    std::optional<ChapterData> LoadChapter(
        const std::string& bookId, int chapter) override;
    const char* ProviderName() const override;
};
```

## Supported Markers

| USFM Marker | Segment Type | Level | Notes |
|-------------|-------------|-------|-------|
| `\id` | (metadata) | — | Book identification, first line |
| `\h` | (metadata) | — | Header/abbreviated title |
| `\mt1`-`\mt4` | BookTitle | 1-4 | Major title levels |
| `\c` | ChapterLabel | — | Chapter number |
| `\s1`-`\s5` | SectionHeading | 1-5 | Section headings |
| `\r` | SectionHeading | 1 | Parallel reference heading |
| `\p` | ParagraphBreak | — | Paragraph break |
| `\q1`-`\q3` | PoetryLine | 1-3 | Poetry/indented lines |
| `\q` | PoetryLine | 1 | Poetry (default level 1) |
| `\v` | (verse boundary) | — | Verse number within VerseText segments |
| `\f ... \f*` | (stripped) | — | Footnotes (ignored in MVP) |

## Output

```cpp
// Input: GEN.usfm, chapter 1
ChapterData {
    bookId: "GEN",
    chapterNum: 1,
    words: [
        {id: 0,     verseId: 1, text: "No"},
        {id: 1,     verseId: 1, text: "princípio"},
        ...
        {id: 38,    verseId: 5, text: "dia"},
        {id: 39,    verseId: 6, text: "E"},
        ...
    ],
    segments: [
        {ChapterLabel,    level:0, text: "1",               verseStart: 0,  verseEnd: 0},
        {ParagraphBreak,  level:0},
        {VerseText,       level:0, verseStart: 1,  verseEnd: 5,   startWordIndex: 0,   wordCount: 38},
        {VerseText,       level:0, verseStart: 6,  verseEnd: 8,   startWordIndex: 38,  wordCount: 45},
        ...
    ]
};
```

## File Format

Each USFM file contains one book. File naming convention: `{book_number}_{BOOK_CODE}.usfm` (e.g., `02_GEN.usfm`, `03_EXO.usfm`).

Files are loaded from `assets/usfm/`. The parser reads the file on first access and caches parsed books in memory. Total memory for all 66 books: ~8MB.

## Data Source

**Bíblia Livre** (CC BY 4.0) from ebible.org (ID: porbr2018):
- Portuguese translation based on Almeida 1819 (Textus Receptus)
- All 66 books available in USFM format
- License: Creative Commons Atribuição 4.0 Brasil
- Attribution required: "Bíblia Livre (BLIVRE), CC BY 4.0"

## Parsing Rules

1. **Tokenization**: Split text on whitespace, create `Word` with global sequential ID and current verse ID
2. **Segment creation**: On encountering `\s`, `\p`, `\q`, etc., close current segment and open new one
3. **Verse tracking**: `\v N` updates current verse number; subsequent words get `verseId = N`
4. **Heading text**: `\s1 Heading Text` — the heading text is stored in `Segment::text`, no words are created for it
5. **Footnotes**: `\f ... \f*` content is stripped entirely
6. **Inline markers**: `\add` (added words) and `\wj` (words of Jesus) are stripped in MVP — the text between them is kept

## Error Handling

- Missing USFM file: return `std::nullopt`
- Malformed marker: skip marker, keep text, log warning
- Missing `\c` marker: treat entire file as a single chapter
- Multiple `\c` in one file: split into separate chapter entries
