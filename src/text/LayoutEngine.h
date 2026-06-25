#ifndef LAYOUTENGINE_H
#define LAYOUTENGINE_H

#include <string>
#include <vector>
#include <raylib.h>
#include "data/ChapterProvider.h"

class LayoutEngine {
public:
    LayoutEngine(float maxWidth, const Font& font, float fontSize, float lineSpacing, float scaleFactor = 1.0f);

    ChapterLayout LayoutChapter(const std::string& chapterId, const ChapterData& data);

    float GetFontSize() const;
    void SetFontSize(float size);

    float GetMaxWidth() const;
    void SetMaxWidth(float width);
    void InvalidateCache();

    int HitTestLine(const ChapterLayout& layout, float chapterRelativeY, float screenX) const;

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

    float LayoutWords(const Segment& seg, const ChapterData& data, float startY,
                      std::vector<Line>& lines, float indent, SegmentType spanType);
    float LayoutHeading(const Segment& seg, float startY, std::vector<Line>& lines, float fontScale);
};

#endif // LAYOUTENGINE_H