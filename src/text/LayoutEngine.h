#ifndef LayoutEngine_h
#define LayoutEngine_h

#include <string>
#include <vector>
#include <raylib.h>

struct Word {
    int id;
    int verseId;
    std::string text;
};

struct Span {
    std::string text;
    float x, y;
    float width, height;
    int verseId;
    int startWord;
    int endWord;
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

class LayoutEngine {
public:
    LayoutEngine(float maxWidth, const Font& font, float fontSize, float lineSpacing);

    ChapterLayout layoutChapter(const std::string& chapterId, const std::string& text);

    float getMaxWidth() const;
    void setMaxWidth(float width);
    void invalidateCache();

    std::string getWordAtPosition(float x, float y, float scrollY);

private:
    float maxWidth;
    Font font;
    float fontSize;
    float lineSpacing;

    std::vector<ChapterLayout> cachedLayouts;

    std::vector<Word> tokenize(const std::string& text);
    std::vector<Line> wrapText(const std::vector<Word>& words);
    std::vector<Span> createSpansForLine(const Line& line, const std::vector<Word>& words);
};

#endif // LayoutEngine_h