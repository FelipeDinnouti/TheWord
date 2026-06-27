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

## Verse Number Identifiers (Phase 12)

### Overview

Verse numbers are inserted at layout time as synthetic `Span` objects with `SegmentType::VerseNumber`. This ensures verse number widths are accounted for in word wrapping and positioning is correct at all font sizes.

### Data Model

```cpp
// Added to SegmentType enum
VerseNumber    // A verse number indicator (grey superscript)
```

The span carries:
- `text`: `"1."`, `"2."`, etc. (verse number digit + dot)
- `verseId`: the verse number being marked
- `type`: `SegmentType::VerseNumber`

### Layout Algorithm

In `LayoutWords()`:

```cpp
int currentVerse = 0;
for each word at index i:
    const Word& word = data.words[i];
    if (word.verseId != currentVerse) {
        currentVerse = word.verseId;

        // Insert verse number span before first word of new verse
        std::string vnText = std::to_string(word.verseId) + ".";
        float vnSize = fontSize * VERSE_NUMBER_SCALE;
        float vnWidth = MeasureTextEx(font, vnText.c_str(), vnSize, 1).x;

        Span vnSpan;
        vnSpan.text = vnText;
        vnSpan.x = x;
        vnSpan.y = y;
        vnSpan.width = vnWidth;
        vnSpan.height = vnSize;
        vnSpan.verseId = word.verseId;
        vnSpan.startWord = -1;     // no word association
        vnSpan.endWord = -1;
        vnSpan.type = SegmentType::VerseNumber;

        currentLine.spans.push_back(vnSpan);
        x += vnWidth + spaceWidth;  // space after verse number + dot
    }
    // ... lay out the word normally
```

### Edge Cases

- **Verse 1 of each chapter**: Always numbered (no "start of chapter means no verse number" rule)
- **Verse spanning multiple layout lines**: The number appears once at the start of the verse on the first line
- **No verse number before headings**: VerseText segments only — SectionHeading, BookTitle, ChapterLabel are not verse-numbered
- **Consecutive same-verse words after a line break**: No repeated verse number (the if-block only triggers on verseId change)

### Rendering

In `Renderer::DrawSpan()`:

```cpp
case SegmentType::VerseNumber:
    float vnSize = fontSize * theme::FONT_VERSE_NUMBER;  // 0.65
    float vnOffsetY = -4.0f * scale;  // superscript Y offset
    DrawTextEx(bodyFont, span.text.c_str(),
               {span.x, screenY + vnOffsetY},
               vnSize, 1, theme::DOC_VERSE_NUMBER);
    break;
```

- Font: `bodyFont` (same as verse text)
- Scale: `theme::FONT_VERSE_NUMBER` = 0.65
- Color: `theme::DOC_VERSE_NUMBER` (grey, e.g. `{160, 160, 160, 255}`)
- Y offset: -4px (scaled) for superscript positioning

### Theme Constants

```cpp
constexpr Color DOC_VERSE_NUMBER = {160, 160, 160, 255};
constexpr float FONT_VERSE_NUMBER = 0.65f;
```

### Future Options

- Allow toggling verse numbers on/off in settings
- Bold verse number for verse 1 of each chapter
- Clickable verse numbers (select all words in verse)

## Key Details

- Line height = `fontSize * lineSpacing`
- Left margin: 10px
- Right margin: 10px (maxWidth accounts for this)
- Poetry indent: `level * 20px`
- Paragraph gap: 8px
- Heading top gap: 12px, bottom gap: 6px
- Each word is rendered as its own span (for now)
- Verse numbers are inserted as synthetic spans at verse transitions
- Tokenization happens in the providers (USFMParser/BibleClient), not in LayoutEngine
