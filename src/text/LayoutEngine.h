#ifndef LAYOUTENGINE_H
#define LAYOUTENGINE_H

#include <string>
#include <vector>
#include <raylib.h>
#include "data/ChapterProvider.h"

class LayoutEngine {
public:
    LayoutEngine(float maxWidth, const Font& font, float fontSize, float lineSpacing);

    ChapterLayout layoutChapter(const std::string& chapterId, const ChapterData& data);

    float getFontSize() const;
    void setFontSize(float size);

    float getMaxWidth() const;
    void setMaxWidth(float width);
    void invalidateCache();

    int hitTestLine(const ChapterLayout& layout, float chapterRelativeY, float screenX) const;

private:
    float maxWidth;
    const Font& font;
    float fontSize;
    float lineSpacing;

    std::vector<ChapterLayout> cachedLayouts;

    float layoutWords(const Segment& seg, const ChapterData& data, float startY,
                      std::vector<Line>& lines, float indent, SegmentType spanType);
    float layoutHeading(const Segment& seg, float startY, std::vector<Line>& lines);

    static constexpr float PARAGRAPH_GAP = 8.0f;
    static constexpr float HEADING_TOP_GAP = 12.0f;
    static constexpr float HEADING_BOTTOM_GAP = 6.0f;
    static constexpr float POETRY_INDENT = 20.0f;
    static constexpr float LEFT_MARGIN = 10.0f;
    static constexpr float RIGHT_MARGIN = 10.0f;
};

#endif // LAYOUTENGINE_H