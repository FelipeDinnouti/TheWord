#ifndef CHAPTER_PROVIDER_H
#define CHAPTER_PROVIDER_H

#include <string>
#include <vector>
#include <optional>

namespace theword::data {

struct Footnote {
    int verseId;
    std::string callerRef;
    std::string text;
};

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
    BookTitle,
    VerseNumber,
    FootnoteMarker
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

struct ChapterData {
    std::string bookId;
    int chapterNum;
    std::vector<Word> words;
    std::vector<Segment> segments;
    std::vector<Footnote> footnotes;
};

class ChapterProvider {
public:
    virtual ~ChapterProvider() = default;
    virtual std::optional<ChapterData> LoadChapter(
        const std::string& bookId, int chapter) = 0;
    virtual const char* ProviderName() const = 0;
};

} // namespace theword::data

#endif
