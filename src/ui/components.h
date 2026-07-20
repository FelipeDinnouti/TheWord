#ifndef UI_COMPONENTS_H
#define UI_COMPONENTS_H

#include "core/UIScale.h"
#include "core/ThemeManager.h"
#include <raylib.h>
#include <string>

namespace theword::ui {

void DrawHeaderBar(const Font& font, float fontSize, const char* title,
                   bool hasBack, int screenWidth, const theword::core::UIScale& uiScale,
                   const theword::core::ThemePalette& palette);

bool StartsWithIgnoreCase(const std::string& str, const std::string& prefix);

bool DrawButton(Rectangle bounds, const char* text, const Font& font, float fontSize,
                const theword::core::ThemePalette& palette,
                bool enabled = true, Color textColor = WHITE, Color bgColor = WHITE);

void DrawPanel(Rectangle bounds, Color bgColor, Color borderColor,
               float rounding = 6.0f, float borderThick = 1.0f);

bool DrawTextItem(Rectangle bounds, const char* text, const Font& font, float fontSize,
                  const theword::core::ThemePalette& palette,
                  bool selected = false, Color textColor = GRAY, Color selTextColor = BLACK);

bool DrawToggle(Rectangle bounds, const theword::core::ThemePalette& palette,
                Color accentColor, bool& value);

bool DrawColorSwatch(Rectangle bounds, Color color, const theword::core::ThemePalette& palette,
                     bool selected = false);

} // namespace theword::ui

#endif
