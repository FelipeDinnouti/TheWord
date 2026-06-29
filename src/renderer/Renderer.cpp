#include "Renderer.h"
#include "event/EventBus.h"
#include "event/Events.h"
#include <algorithm>

namespace theword::renderer {

using namespace theword::core;
using namespace theword::data;

Renderer::Renderer(theword::event::EventBus& eventBus,
                   const Font& bodyFont, const Font& headingFont,
                   const Font& largeFont, const Font& smallFont,
                   float contentTop,
                   float bodySize, float headingSize,
                   float largeSize, float smallSize,
                   float dpiScale)
    : eventBus_(eventBus), bodyFont(bodyFont), headingFont(headingFont),
      largeFont(largeFont), smallFont(smallFont),
      contentTop(contentTop),
      bodySize_(bodySize), headingSize_(headingSize),
      largeSize_(largeSize), smallSize_(smallSize),
      dpiScale_(dpiScale) {}

void Renderer::SetFontSizes(float body, float heading, float large, float small) {
    bodySize_ = body;
    headingSize_ = heading;
    largeSize_ = large;
    smallSize_ = small;
}

float Renderer::GetFontSize() const {
    return bodySize_;
}

float Renderer::GetContentTop() const {
    return contentTop;
}

void Renderer::DrawFrame(float scrollY, float totalHeight, float viewportHeight,
                          const std::vector<std::pair<Span, float>>& docSpans,
                          const std::vector<HighlightRect>& highlightRects) {
    for (const auto& hr : highlightRects) {
        DrawRectangle(hr.x, hr.y, hr.width, hr.height, hr.color);
    }

    for (const auto& [span, docY] : docSpans) {
        float screenY = docY - scrollY + contentTop;
        if (screenY < -CULL_MARGIN || screenY > GetScreenHeight()) continue;
        DrawSpan(span, screenY);
    }

    if (totalHeight > viewportHeight) {
        DrawScrollbar(scrollY, totalHeight, viewportHeight);
    }
}

void Renderer::DrawSpan(const Span& span, float screenY) {
    Color color = theme::DOC_BODY;
    float drawSize = bodySize_;
    Font useFont = bodyFont;

    switch (span.type) {
        case SegmentType::BookTitle:
            color = theme::DOC_BOOK_TITLE;
            drawSize = largeSize_;
            useFont = largeFont;
            break;
        case SegmentType::ChapterLabel:
            color = theme::DOC_CHAPTER_LABEL;
            drawSize = largeSize_;
            useFont = largeFont;
            break;
        case SegmentType::SectionHeading:
            color = theme::DOC_HEADING;
            drawSize = headingSize_;
            useFont = headingFont;
            break;
        case SegmentType::PoetryLine:
            color = theme::DOC_POETRY;
            drawSize = bodySize_;
            break;
        case SegmentType::VerseNumber:
            color = theme::DOC_VERSE_NUMBER;
            drawSize = smallSize_;
            useFont = smallFont;
            break;
        default:
            break;
    }

    Vector2 pos = {span.x, screenY};
    if (span.type == SegmentType::VerseNumber) {
        pos.y = screenY - drawSize * 0.25f;
    }
    DrawTextEx(useFont, span.text.c_str(), pos, drawSize, 1, color);
}

void Renderer::DrawScrollbar(float scrollY, float totalHeight, float viewportHeight) {
    float scrollBarHeight = viewportHeight * (viewportHeight / totalHeight);
    float minH = MIN_SCROLLBAR_HEIGHT * dpiScale_;
    if (scrollBarHeight < minH) scrollBarHeight = minH;

    float scrollBarY = contentTop + (scrollY / totalHeight) * (viewportHeight - scrollBarHeight);
    float barWidth = 6.0f * dpiScale_;
    float rightGap = 8.0f * dpiScale_;
    DrawRectangle(GetScreenWidth() - static_cast<int>(rightGap),
                  static_cast<int>(scrollBarY),
                  static_cast<int>(barWidth),
                  static_cast<int>(scrollBarHeight),
                  theme::SCROLLBAR_THUMB);
}

void Renderer::DrawFpsCounter(int x, int y) {
    DrawFPS(x, y);
}

} // namespace theword::renderer
