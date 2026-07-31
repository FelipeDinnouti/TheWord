#ifndef LAYOUT_TYPES_H
#define LAYOUT_TYPES_H

#include <string>
#include <vector>
#include "data/ChapterProvider.h"

namespace theword::text {

struct Span {
    std::string text;
    float x = 0.0f, y = 0.0f;
    float width = 0.0f, height = 0.0f;
    int verseId = 0;
    int startWord = 0;
    int endWord = 0;
    std::string bookId;
    int chapterNum = 0;
    int footnoteIndex = -1;
    theword::data::SegmentType type = theword::data::SegmentType::VerseText;
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

} // namespace theword::text

#endif // LAYOUT_TYPES_H
