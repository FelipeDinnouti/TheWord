#ifndef LAYOUT_ENGINE_H
#define LAYOUT_ENGINE_H

#include <string>
#include <vector>
#include <raylib.h>
#include "data/ChapterProvider.h"

namespace theword::event {
    class EventBus;
    struct ResizeEvent;
    struct FontSizeEvent;
}

namespace theword::text {

class LayoutEngine {
public:
    LayoutEngine(theword::event::EventBus& eventBus,
                 float maxWidth, const Font& font, float fontSize,
                 float lineSpacing, float scaleFactor = 1.0f);

    theword::data::ChapterLayout LayoutChapter(const std::string& chapterId, const theword::data::ChapterData& data);

    float GetFontSize() const;
    void SetFontSize(float size);

    float GetMaxWidth() const;
    void SetMaxWidth(float width);
    void InvalidateCache();

    int HitTestLine(const theword::data::ChapterLayout& layout, float chapterRelativeY, float screenX) const;

private:
    theword::event::EventBus& eventBus_;
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

    std::vector<theword::data::ChapterLayout> cachedLayouts;

    void OnResize(const theword::event::ResizeEvent& e);
    void OnFontSize(const theword::event::FontSizeEvent& e);

    float LayoutWords(const theword::data::Segment& seg, const theword::data::ChapterData& data, float startY,
                      std::vector<theword::data::Line>& lines, float indent, theword::data::SegmentType spanType);
    float LayoutHeading(const theword::data::Segment& seg, float startY, std::vector<theword::data::Line>& lines, float fontScale);
};

} // namespace theword::text

#endif // LAYOUTENGINE_H
