#ifndef RENDERER_H
#define RENDERER_H

#include <string>
#include <vector>
#include <raylib.h>
#include "renderer/DrawContext.h"
#include "text/LayoutTypes.h"
#include "highlight/PersistenceInterface.h"

namespace theword::renderer {

struct HighlightRect {
    float x;
    float y;
    float width;
    float height;
    theword::highlight::SimpleColor color;
};

class Renderer {
public:
    explicit Renderer(float contentTop);

    void DrawFrame(const DrawContext& ctx, float scrollY, float totalHeight, float viewportHeight,
                   const std::vector<std::pair<theword::text::Span, float>>& docSpans,
                   const std::vector<HighlightRect>& highlightRects = {});
    void DrawScrollbar(const DrawContext& ctx, float scrollY, float totalHeight, float viewportHeight);

    void DrawFpsCounter(int x, int y);

    float GetContentTop() const;

public:
    static constexpr float CULL_MARGIN = 50.0f;

private:
    static constexpr float MIN_SCROLLBAR_HEIGHT = 20.0f;

    const float contentTop;

    void DrawSpan(const DrawContext& ctx, const theword::text::Span& span, float screenY);
};

} // namespace theword::renderer

#endif
