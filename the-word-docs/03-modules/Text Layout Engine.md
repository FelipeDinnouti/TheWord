# Text Layout Engine

> Status: Updated 2026-06-22

Files: `src/text/LayoutEngine.h/cpp`

## Overview

The layout engine is the heart of the text rendering system. It takes `ChapterData` (tokens + segments) from any `ChapterProvider` and produces a complete `ChapterLayout` with positioned lines and spans in document-space coordinates.

## Interface

```cpp
class LayoutEngine {
public:
    LayoutEngine(float maxWidth, const Font& font, float fontSize, float lineSpacing);

    ChapterLayout layoutChapter(const std::string& chapterId, const ChapterData& data);

    float getMaxWidth() const;
    void setMaxWidth(float width);
    void invalidateCache();

    int getWordAtPosition(float x, float y, float scrollY);

private:
    float layoutVerseText(const Segment& seg, const ChapterData& data,
                          float& currentY, std::vector<Line>& lines);
    float layoutHeading(const Segment& seg, float& currentY, std::vector<Line>& lines);
    float layoutPoetryLine(const Segment& seg, const ChapterData& data,
                           float& currentY, std::vector<Line>& lines);
};
```

## Layout Algorithm

### Input: ChapterData

```
segments: [
    {SectionHeading, level:1, text: "A Criação"},
    {ParagraphBreak},
    {VerseText, verseStart:1, verseEnd:5, startWordIndex:0, wordCount:38},
    {PoetryLine, level:1, verseStart:27, startWordIndex:100, wordCount:6},
    {PoetryLine, level:2, verseStart:27, startWordIndex:106, wordCount:5},
]
```

### Segment Layout

Each segment type is laid out differently:

| SegmentType | Layout |
|-------------|--------|
| **VerseText** | Left-aligned, word-wrapped within `maxWidth`. 10px margins. |
| **SectionHeading** | Centered, rendered with `headingFont` at `headingSize` (31px), single line. Block-level spacing above/below. |
| **ParagraphBreak** | Insert vertical gap (8px), no line generated. |
| **PoetryLine** | Left-indented by `level * 20px`. Wrapped with reduced width. |
| **ChapterLabel** | Large centered text, block-level spacing. |
| **BookTitle** | Very large centered text, block-level spacing. |

### Word Wrapping (VerseText)

Words are placed on lines left to right. When adding the next word would exceed the maximum width, a new line is started.

**Measurement:** Each word is measured using `MeasureTextEx(bodyFont, word, fontSize, 1)`. Space width is also measured for inter-word spacing. The layout engine only uses `bodyFont` — heading measurement is handled by the renderer.

### Span Generation

Each word becomes its own span (current simplification). Span position and size match the word's measured dimensions. In the future, multiple words may be merged into a single span for rendering efficiency.

### Segment Flow

```
currentY = chapterStartY
for each segment in segments:
    switch segment.type:
        case ParagraphBreak:
            currentY += PARAGRAPH_GAP (8px)
        case SectionHeading:
            currentY += HEADING_TOP_GAP (12px)
            render heading text centered at currentY
            currentY += headingHeight
            currentY += HEADING_BOTTOM_GAP (6px)
        case VerseText:
            wordWrap(words[segment.startWordIndex..end], currentY)
            currentY += totalLineHeight
        case PoetryLine:
            indent = segment.level * 20
            wordWrap(words[start..end], maxWidth - indent, currentY)
            currentY += lineHeight
```

### Step 4: Caching

Complete `ChapterLayout` objects are cached by chapter ID. `layoutChapter()` returns the cached version if available, eliminating redundant layout computation.

**Cache invalidation:** Call `invalidateCache()` when the window width or font size changes. This clears all cached layouts.

## Key Details

- Line height = `fontSize * lineSpacing`
- Left margin: 10px
- Right margin: 10px (maxWidth accounts for this)
- Poetry indent: `level * 20px`
- Paragraph gap: 8px
- Heading top gap: 12px, bottom gap: 6px
- Each word is rendered as its own span (for now)
- Tokenization now happens in the providers (USFMParser/BibleClient), not in LayoutEngine
