#include "Renderer.h"
#include <algorithm>

Renderer::Renderer(const Font& bodyFont, const Font& headingFont, float contentTop, float fontSize)
    : bodyFont(bodyFont), headingFont(headingFont), contentTop(contentTop), fontSize(fontSize), headingSize(fontSize * 1.3f) {}

float Renderer::getContentTop() const {
    return contentTop;
}

void Renderer::drawFrame(float scrollY, float totalHeight, float viewportHeight,
                          const std::vector<std::pair<Span, float>>& docSpans,
                          const std::string& chapterTitle,
                          const std::vector<HighlightRect>& highlightRects) {
    if (!chapterTitle.empty()) {
        DrawTextEx(headingFont, chapterTitle.c_str(), {20, 20}, headingSize, 1, DARKGRAY);
    }

    for (const auto& hr : highlightRects) {
        DrawRectangle(hr.x, hr.y, hr.width, hr.height, hr.color);
    }

    for (const auto& [span, docY] : docSpans) {
        float screenY = docY - scrollY + contentTop;
        if (screenY < -50.0f || screenY > GetScreenHeight()) continue;
        drawSpan(span, screenY);
    }

    if (totalHeight > viewportHeight) {
        drawScrollbar(scrollY, totalHeight, viewportHeight);
    }
}

void Renderer::drawSpan(const Span& span, float screenY) {
    Color color = BLACK;
    float drawSize = fontSize;
    Font useFont = bodyFont;

    switch (span.type) {
        case SegmentType::SectionHeading:
        case SegmentType::ChapterLabel:
        case SegmentType::BookTitle:
            color = DARKGRAY;
            drawSize = headingSize;
            useFont = headingFont;
            break;
        case SegmentType::PoetryLine:
            color = DARKGRAY;
            drawSize = fontSize;
            break;
        default:
            break;
    }

    DrawTextEx(useFont, span.text.c_str(), {span.x, screenY}, drawSize, 1, color);
}

void Renderer::drawScrollbar(float scrollY, float totalHeight, float viewportHeight) {
    float scrollBarHeight = viewportHeight * (viewportHeight / totalHeight);
    if (scrollBarHeight < 20.0f) scrollBarHeight = 20.0f;

    float scrollBarY = contentTop + (scrollY / totalHeight) * (viewportHeight - scrollBarHeight);
    DrawRectangle(GetScreenWidth() - 6, (int)scrollBarY, 4, (int)scrollBarHeight, LIGHTGRAY);
}

void Renderer::drawFpsCounter(int x, int y) {
    DrawFPS(x, y);
}
