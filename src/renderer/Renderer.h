#ifndef RENDERER_H
#define RENDERER_H

#include <string>
#include <vector>
#include <raylib.h>
#include "data/ChapterProvider.h"
#include "highlight/PersistenceInterface.h"

namespace theword::core { class ThemeManager; }

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
    Renderer(const Font& bodyFont, const Font& headingFont,
             const Font& largeFont, const Font& smallFont,
             float contentTop,
             float bodySize, float headingSize,
             float largeSize, float smallSize,
             float dpiScale,
             const theword::core::ThemeManager& themeManager);

    void DrawFrame(float scrollY, float totalHeight, float viewportHeight,
                   const std::vector<std::pair<theword::data::Span, float>>& docSpans,
                   const std::vector<HighlightRect>& highlightRects = {});
    void DrawScrollbar(float scrollY, float totalHeight, float viewportHeight);
    void SetFontSizes(float body, float heading, float large, float small);
    float GetFontSize() const;

    void DrawFpsCounter(int x, int y);

    float GetContentTop() const;

public:
    static constexpr float CULL_MARGIN = 50.0f;

private:
    static constexpr float MIN_SCROLLBAR_HEIGHT = 20.0f;

    const Font& bodyFont;
    const Font& headingFont;
    const Font& largeFont;
    const Font& smallFont;
    const float contentTop;
    float bodySize_;
    float headingSize_;
    float largeSize_;
    float smallSize_;
    const float dpiScale_;
    const theword::core::ThemeManager& themeManager_;

    void DrawSpan(const theword::data::Span& span, float screenY);
};

} // namespace theword::renderer

#endif
