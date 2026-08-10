#ifndef UI_COMPONENTS_H
#define UI_COMPONENTS_H

#include "renderer/DrawContext.h"
#include <raylib.h>
#include <string>

namespace theword::ui {

void DrawHeaderBar(const theword::renderer::DrawContext& ctx, const Font& font, float fontSize,
                   const char* title, bool hasBack);

bool StartsWithIgnoreCase(const std::string& str, const std::string& prefix);

bool PointInRect(float x, float y, Rectangle r);

bool DrawButton(const theword::renderer::DrawContext& ctx, Rectangle bounds, const char* text,
                const Font& font, float fontSize, bool enabled = true,
                Color textColor = WHITE, Color bgColor = WHITE);

void DrawPanel(Rectangle bounds, Color bgColor, Color borderColor,
               float rounding = 6.0f, float borderThick = 1.0f);

bool DrawTextItem(const theword::renderer::DrawContext& ctx, Rectangle bounds, const char* text,
                  const Font& font, float fontSize, bool selected = false,
                  Color textColor = GRAY, Color selTextColor = BLACK);

bool DrawToggle(const theword::renderer::DrawContext& ctx, Rectangle bounds,
                Color accentColor, bool& value);

bool DrawColorSwatch(const theword::renderer::DrawContext& ctx, Rectangle bounds, Color color,
                     bool selected = false);

} // namespace theword::ui

#endif
