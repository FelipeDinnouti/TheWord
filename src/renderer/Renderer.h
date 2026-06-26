#ifndef RENDERER_H
#define RENDERER_H

#include <string>
#include <vector>
#include <raylib.h>
#include "data/ChapterProvider.h"
#include "core/Theme.h"

namespace theword::event {
    class EventBus;
    struct FontSizeEvent;
}

namespace theword::renderer {

struct HighlightRect {
    float x;
    float y;
    float width;
    float height;
    Color color;
};

class Renderer {
public:
    Renderer(theword::event::EventBus& eventBus,
             const Font& bodyFont, const Font& headingFont,
             float contentTop, float fontSize);

    void DrawFrame(float scrollY, float totalHeight, float viewportHeight,
                   const std::vector<std::pair<theword::data::Span, float>>& docSpans,
                   const std::vector<HighlightRect>& highlightRects = {});
    void DrawScrollbar(float scrollY, float totalHeight, float viewportHeight);
    void SetFontSize(float size);
    float GetFontSize() const;

    void DrawFpsCounter(int x, int y);

    float GetContentTop() const;

private:
    theword::event::EventBus& eventBus_;
    static constexpr float CULL_MARGIN = 50.0f;
    static constexpr float MIN_SCROLLBAR_HEIGHT = 20.0f;

    const Font& bodyFont;
    const Font& headingFont;
    float contentTop;
    float fontSize;
    float headingSize;

    void OnFontSize(const theword::event::FontSizeEvent& e);
    void DrawSpan(const theword::data::Span& span, float screenY);
};

} // namespace theword::renderer

#endif
