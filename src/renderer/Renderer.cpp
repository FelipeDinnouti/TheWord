#include "Renderer.h"
#include <algorithm>

Renderer::Renderer(const Font& bodyFont, const Font& headingFont, float contentTop, float fontSize)
    : bodyFont(bodyFont), headingFont(headingFont), contentTop(contentTop), fontSize(fontSize), headingSize(fontSize * theme::FONT_HEADING) {}

void Renderer::setFontSize(float size) {
    fontSize = size;
    headingSize = size * theme::FONT_HEADING;
}

float Renderer::getFontSize() const {
    return fontSize;
}

float Renderer::getContentTop() const {
    return contentTop;
}

void Renderer::drawFrame(float scrollY, float totalHeight, float viewportHeight,
                          const std::vector<std::pair<Span, float>>& docSpans,
                          const std::vector<HighlightRect>& highlightRects) {
    for (const auto& hr : highlightRects) {
        DrawRectangle(hr.x, hr.y, hr.width, hr.height, hr.color);
    }

    for (const auto& [span, docY] : docSpans) {
        float screenY = docY - scrollY + contentTop;
        if (screenY < -CULL_MARGIN || screenY > GetScreenHeight()) continue;
        drawSpan(span, screenY);
    }

    if (totalHeight > viewportHeight) {
        drawScrollbar(scrollY, totalHeight, viewportHeight);
    }
}

void Renderer::drawSpan(const Span& span, float screenY) {
    Color color = theme::DOC_BODY;
    float drawSize = fontSize;
    Font useFont = bodyFont;

    switch (span.type) {
        case SegmentType::BookTitle:
            color = theme::DOC_BOOK_TITLE;
            drawSize = fontSize * theme::FONT_LARGE_HEADING;
            useFont = headingFont;
            break;
        case SegmentType::ChapterLabel:
            color = theme::DOC_CHAPTER_LABEL;
            drawSize = fontSize * theme::FONT_LARGE_HEADING;
            useFont = headingFont;
            break;
        case SegmentType::SectionHeading:
            color = theme::DOC_HEADING;
            drawSize = headingSize;
            useFont = headingFont;
            break;
        case SegmentType::PoetryLine:
            color = theme::DOC_POETRY;
            drawSize = fontSize;
            break;
        default:
            break;
    }

    DrawTextEx(useFont, span.text.c_str(), {span.x, screenY}, drawSize, 1, color);
}

void Renderer::drawScrollbar(float scrollY, float totalHeight, float viewportHeight) {
    float scrollBarHeight = viewportHeight * (viewportHeight / totalHeight);
    if (scrollBarHeight < MIN_SCROLLBAR_HEIGHT) scrollBarHeight = MIN_SCROLLBAR_HEIGHT;

    float scrollBarY = contentTop + (scrollY / totalHeight) * (viewportHeight - scrollBarHeight);
    DrawRectangle(GetScreenWidth() - 8, (int)scrollBarY, 6, (int)scrollBarHeight, theme::SCROLLBAR_THUMB);
}

void Renderer::drawFpsCounter(int x, int y) {
    DrawFPS(x, y);
}
