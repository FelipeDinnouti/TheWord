#ifndef DRAW_CONTEXT_H
#define DRAW_CONTEXT_H

#include "core/InputFrame.h"
#include "core/UIScale.h"
#include "core/ThemeManager.h"
#include "renderer/FontManager.h"

namespace theword::renderer {

struct DrawContext {
    const theword::core::ThemeManager& themeManager;
    const FontManager& fonts;
    const theword::core::UIScale& uiScale;
    const theword::core::InputFrame& input;
    float scale;

    void PushClipRect(float x, float y, float w, float h);
    void PopClipRect();
    void SetCursor(int cursorKind);
};

} // namespace theword::renderer

#endif