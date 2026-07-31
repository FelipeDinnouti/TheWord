#ifndef LAYOUT_ENGINE_H
#define LAYOUT_ENGINE_H

#include <string>
#include <vector>
#include <atomic>
#include "data/ChapterProvider.h"
#include "text/LayoutTypes.h"
#include "text/TextMeasure.h"

namespace theword::event {
    class EventBus;
    struct ResizeEvent;
    struct FontSizeEvent;
}

namespace theword::text {

class LayoutEngine {
public:
    LayoutEngine(theword::event::EventBus& eventBus,
                 float maxWidth,
                 TextMeasureFn measureFn,
                 float bodySize, float headingSize,
                 float largeSize, float smallSize,
                 float lineSpacing, float scaleFactor = 1.0f);

    ChapterLayout LayoutChapter(const std::string& chapterId, const theword::data::ChapterData& data, bool skipCache = false);

    int GetGeneration() const { return layoutGeneration_; }

    float GetFontSize() const;
    void SetFontSizes(float body, float heading, float large, float small);

    float GetMaxWidth() const;
    void SetMaxWidth(float width);
    void InvalidateCache();

    int HitTestLine(const ChapterLayout& layout, float chapterRelativeY, float screenX) const;

private:
    theword::event::EventBus& eventBus_;
    float maxWidth;
    TextMeasureFn measureFn_;
    float bodySize_;
    float headingSize_;
    float largeSize_;
    float smallSize_;
    float lineSpacing;
    const float leftMargin;
    const float rightMargin;
    const float paragraphGap;
    const float headingTopGap;
    const float headingBottomGap;
    const float poetryIndent;

    std::vector<ChapterLayout> cachedLayouts;
    std::atomic<int> layoutGeneration_{0};

    void OnResize(const theword::event::ResizeEvent& e);

    float LayoutWords(const theword::data::Segment& seg, const theword::data::ChapterData& data, float startY,
                      std::vector<Line>& lines, float indent, theword::data::SegmentType spanType);
    float LayoutHeading(const theword::data::Segment& seg, float startY, std::vector<Line>& lines,
                        FontKind kind, float renderSize);
};

} // namespace theword::text

#endif // LAYOUTENGINE_H
