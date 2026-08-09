#include "Renderer.h"
#include "core/Theme.h"
#include "core/ThemeManager.h"

namespace theword::renderer {

using namespace theword::core;
using namespace theword::data;
using namespace theword::text;

Renderer::Renderer(float contentTop)
    : contentTop(contentTop) {}

float Renderer::GetContentTop() const {
    return contentTop;
}

void Renderer::DrawFrame(const DrawContext& ctx, float scrollY, float totalHeight,
                         float viewportHeight,
                         const std::vector<std::pair<Span, float>>& docSpans,
                         const std::vector<HighlightRect>& highlightRects) {
    for (const auto& hr : highlightRects) {
        if (hr.y < -CULL_MARGIN || hr.y > GetScreenHeight()) continue;
        DrawRectangle(hr.x, hr.y, hr.width, hr.height,
                      Color{hr.color.r, hr.color.g, hr.color.b, hr.color.a});
    }

    for (const auto& [span, docY] : docSpans) {
        float screenY = docY - scrollY + contentTop;
        if (screenY < -CULL_MARGIN || screenY > GetScreenHeight()) continue;
        DrawSpan(ctx, span, screenY);
    }

    if (totalHeight > viewportHeight) {
        DrawScrollbar(ctx, scrollY, totalHeight, viewportHeight);
    }
}

void Renderer::DrawSpan(const DrawContext& ctx, const Span& span, float screenY) {
    const auto& palette = ctx.themeManager.Current();
    const FontManager& fonts = ctx.fonts;
    Color color = palette.docBody;
    float drawSize = fonts.BodySize();
    Font useFont = fonts.Get(FontKind::Body);

    switch (span.type) {
        case SegmentType::BookTitle:
            color = palette.docBookTitle;
            drawSize = fonts.LargeSize();
            useFont = fonts.Get(FontKind::Large);
            break;
        case SegmentType::ChapterLabel:
            color = palette.docChapterLabel;
            drawSize = fonts.LargeSize();
            useFont = fonts.Get(FontKind::Large);
            break;
        case SegmentType::SectionHeading:
            color = palette.docHeading;
            drawSize = fonts.HeadingFontSize();
            useFont = fonts.Get(FontKind::Heading);
            break;
        case SegmentType::PoetryLine:
            color = palette.docPoetry;
            drawSize = fonts.BodySize();
            break;
        case SegmentType::VerseNumber:
            color = palette.docVerseNumber;
            drawSize = fonts.SmallSize();
            useFont = fonts.Get(FontKind::Small);
            break;
        case SegmentType::FootnoteMarker:
            color = palette.docFootnoteCaller;
            drawSize = fonts.SmallSize();
            useFont = fonts.Get(FontKind::Small);
            break;
        default:
            break;
    }

    Vector2 pos = {span.x, screenY};
    if (span.type == SegmentType::VerseNumber || span.type == SegmentType::FootnoteMarker) {
        pos.y = screenY - drawSize * 0.25f;
    }
    DrawTextEx(useFont, span.text.c_str(), pos, drawSize, 1, color);
}

void Renderer::DrawScrollbar(const DrawContext& ctx, float scrollY, float totalHeight,
                             float viewportHeight) {
    float dpiScale = ctx.scale;
    float scrollBarHeight = viewportHeight * (viewportHeight / totalHeight);
    float minH = MIN_SCROLLBAR_HEIGHT * dpiScale;
    if (scrollBarHeight < minH) scrollBarHeight = minH;

    float scrollBarY = contentTop + (scrollY / totalHeight) * (viewportHeight - scrollBarHeight);
    float barWidth = 6.0f * dpiScale;
    float rightGap = 8.0f * dpiScale;
    float roundness = 2.0f / (barWidth * 0.5f);
    if (roundness > 1.0f) roundness = 1.0f;
    const auto& palette = ctx.themeManager.Current();
    DrawRectangleRounded(
        {GetScreenWidth() - rightGap, scrollBarY, barWidth, scrollBarHeight},
        roundness, 8, palette.scrollbarThumb);
}

void Renderer::DrawFpsCounter(int x, int y) {
    DrawFPS(x, y);
}

} // namespace theword::renderer
