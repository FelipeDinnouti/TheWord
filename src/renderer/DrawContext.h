#ifndef DRAW_CONTEXT_H
#define DRAW_CONTEXT_H

#include "core/UIScale.h"
#include "core/ThemeManager.h"
#include "renderer/FontManager.h"

namespace theword::renderer {

struct DrawContext {
    const theword::core::ThemeManager& themeManager;
    const FontManager& fonts;
    const theword::core::UIScale& uiScale;
    float scale;
};

} // namespace theword::renderer

#endif
