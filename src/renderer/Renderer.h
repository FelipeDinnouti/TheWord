#ifndef RENDERER_H
#define RENDERER_H

#include <string>
#include <vector>
#include <raylib.h>
#include "data/ChapterProvider.h"

struct HighlightRect {
    float x;
    float y;
    float width;
    float height;
    Color color;
};

class Renderer {
public:
    Renderer(const Font& bodyFont, const Font& headingFont, float contentTop, float fontSize);

    void drawFrame(float scrollY, float totalHeight, float viewportHeight,
                   const std::vector<std::pair<Span, float>>& docSpans,
                   const std::vector<HighlightRect>& highlightRects = {});
    void drawScrollbar(float scrollY, float totalHeight, float viewportHeight);
    void setFontSize(float size);
    float getFontSize() const;

    void drawFpsCounter(int x, int y);

    float getContentTop() const;

private:
    static constexpr float CULL_MARGIN = 50.0f;
    static constexpr float MIN_SCROLLBAR_HEIGHT = 20.0f;

    const Font& bodyFont;
    const Font& headingFont;
    float contentTop;
    float fontSize;
    float headingSize;

    void drawSpan(const Span& span, float screenY);
};

#endif
