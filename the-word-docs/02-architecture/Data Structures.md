# Core Data Structures

> Status: Updated 2026-06-22

## Word

The smallest unit of text. Every word in the Bible has a globally unique ID — Genesis 1:1 words have lower IDs than Genesis 1:2 words. This flat numbering scheme makes range-based highlighting straightforward and stable across re-layouts.

IDs are assigned by `GetNextWordId()` in `src/core/GlobalId.h` — a single shared counter used by all `ChapterProvider` implementations. This guarantees uniqueness even when `CompositeProvider` falls back between providers.

```cpp
struct Word {
    int id;           // Global word index (unique across entire document)
    int verseId;      // Parent verse reference
    std::string text; // The actual word text
};
```

## Segment

A structural element that describes how a portion of text should be rendered. Segments are produced by both `USFMParser` and `BibleClient`, and consumed by `LayoutEngine` to produce the final layout.

```cpp
enum class SegmentType {
    VerseText,       // Regular verse text, left-aligned, wrapped
    SectionHeading,  // Section title (s1, s2), centered, bold
    ParagraphBreak,  // Vertical spacing between paragraphs
    PoetryLine,      // Poetry/quote (q1, q2, q3), indented
    ChapterLabel,    // Chapter number display
    BookTitle        // Book title (mt1, mt2)
};

struct Segment {
    SegmentType type;
    int level;               // Heading depth (1-5), poetry indent (1-3)
    std::string text;        // For headings: the heading text
    int verseStart;          // Verse range for VerseText segments
    int verseEnd;
    size_t startWordIndex;   // Index into the chapter's Word array
    size_t wordCount;        // Number of words in this segment
};
```

## ChapterData

The output of both `ChapterProvider` implementations. Contains pre-tokenized words with structural segments.

```cpp
struct ChapterData {
    std::string bookId;       // e.g., "GEN"
    int chapterNum;           // e.g., 1
    std::vector<Word> words;  // All words in the chapter (with global IDs)
    std::vector<Segment> segments; // Rich-text structure
};
```

## Span

A contiguous run of text with shared properties. Spans are the unit of rendering and hit detection. Each span stores its position and size in document-space coordinates, the verse it belongs to, and the range of word IDs it covers.

```cpp
struct Span {
    std::string text;
    float x, y;       // Document-space position
    float width, height;
    int verseId;
    int startWord;    // First word index in this span
    int endWord;      // Last word index in this span
};
```

## Line

Groups spans that share the same baseline. Line height is uniform within a chapter.

```cpp
struct Line {
    float y;             // Document-space Y position
    float height;        // Line height (fontSize * lineSpacing)
    std::vector<Span> spans;
};
```

## ChapterLayout

A fully laid-out chapter. Cached by the layout engine keyed by chapter ID.

```cpp
struct ChapterLayout {
    std::string chapterId;       // e.g., "JHN.3"
    float startY;                // Document-space start position
    float totalHeight;           // Total rendered height
    std::vector<Line> lines;
};
```

## LoadedChapter

A chapter loaded into the document manager with its position in document space.

```cpp
struct LoadedChapter {
    std::string chapterId;
    ChapterData data;       // Raw data for re-layout
    ChapterLayout layout;   // Cached layout
    float startY;           // Document-space start Y
    float height;           // Chapter height in document space
};
```

The `data` field replaces the old `rawText` — it stores the full `ChapterData` so that `invalidateLayouts()` can re-layout chapters without re-fetching from the provider.

## ChapterProvider

Abstract interface for both data sources. Defined in `src/data/ChapterProvider.h`.

```cpp
class ChapterProvider {
public:
    virtual ~ChapterProvider() = default;
    virtual bool HasChapter(const std::string& bookId, int chapter) const = 0;
    virtual std::optional<ChapterData> LoadChapter(
        const std::string& bookId, int chapter) = 0;
    virtual const char* ProviderName() const = 0;
};
```

## Highlight

A selected region of text. Defined as a range of word IDs to be resolution-independent.

```cpp
struct Highlight {
    int id;
    int startWord;
    int endWord;
    int typeId;                  // References HighlightType (color)
    std::string providerName;    // Source: "USFMParser" or "BibleClient"
    std::string bookId;          // e.g. "GEN" (Phase 13)
    int chapterNum;              // e.g. 1 (Phase 13)
    int verseStart;              // First verse in range (Phase 13)
    int verseEnd;                // Last verse in range (Phase 13)
};

struct HighlightType {
    int id;
    std::string name;
    SimpleColor color;
};
```
```
