#ifndef CHAPTER_PROVIDER_H
#define CHAPTER_PROVIDER_H

#include <string>
#include <vector>
#include <optional>

struct Word {
    int id;
    int verseId;
    std::string text;
};

enum class SegmentType {
    VerseText,
    SectionHeading,
    ParagraphBreak,
    PoetryLine,
    ChapterLabel,
    BookTitle
};

struct Segment {
    SegmentType type;
    int level;
    std::string text;
    int verseStart;
    int verseEnd;
    size_t startWordIndex;
    size_t wordCount;
};

struct Span {
    std::string text;
    float x, y;
    float width, height;
    int verseId;
    int startWord;
    int endWord;
    SegmentType type;
};

struct Line {
    float y;
    float height;
    std::vector<Span> spans;
};

struct ChapterLayout {
    std::string chapterId;
    float startY;
    float totalHeight;
    std::vector<Line> lines;
};

struct ChapterData {
    std::string bookId;
    int chapterNum;
    std::vector<Word> words;
    std::vector<Segment> segments;
};

class ChapterProvider {
public:
    virtual ~ChapterProvider() = default;
    virtual bool HasChapter(const std::string& bookId, int chapter) const = 0;
    virtual std::optional<ChapterData> LoadChapter(
        const std::string& bookId, int chapter) = 0;
    virtual const char* ProviderName() const = 0;
};

#endif
