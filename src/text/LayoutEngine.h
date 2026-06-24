#ifndef LAYOUTENGINE_H
#define LAYOUTENGINE_H

#include <string>
#include <vector>
#include <raylib.h>
#include "data/ChapterProvider.h"

class LayoutEngine {
public:
    LayoutEngine(float maxWidth, const Font& font, float fontSize, float lineSpacing, float scaleFactor = 1.0f);

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
    float leftMargin;
    float rightMargin;
    float paragraphGap;
    float headingTopGap;
    float headingBottomGap;
    float poetryIndent;

    std::vector<ChapterLayout> cachedLayouts;

    float layoutWords(const Segment& seg, const ChapterData& data, float startY,
                      std::vector<Line>& lines, float indent, SegmentType spanType);
    float layoutHeading(const Segment& seg, float startY, std::vector<Line>& lines, float fontScale);
};

#endif // LAYOUTENGINE_H