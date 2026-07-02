#ifndef UI_COMPONENTS_H
#define UI_COMPONENTS_H

#include "core/UIScale.h"
#include <raylib.h>
#include <string>

namespace theword::ui {

// Draw a header bar at the top of a pushed screen.
void DrawHeaderBar(const Font& font, float fontSize, const char* title,
                   bool hasBack, int screenWidth, const theword::core::UIScale& uiScale);

// Case-insensitive prefix match helper.
bool StartsWithIgnoreCase(const std::string& str, const std::string& prefix);

// Draw a rounded button with label. Handles hover/press states internally.
// Returns true if clicked this frame.
bool DrawButton(Rectangle bounds, const char* text, const Font& font, float fontSize,
                bool enabled = true, Color textColor = WHITE, Color bgColor = WHITE);

// Draw a rounded panel container with optional border stroke.
void DrawPanel(Rectangle bounds, Color bgColor, Color borderColor,
               float rounding = 6.0f, float borderThick = 1.0f);

// Draw a selectable list item row. Detects hover/press internally.
// 'selected' is the semantic selection state provided by the caller.
// Returns true if clicked this frame.
bool DrawTextItem(Rectangle bounds, const char* text, const Font& font, float fontSize,
                  bool selected = false, Color textColor = GRAY, Color selTextColor = BLACK);

// Draw a toggle switch. 'value' is toggled internally when clicked.
// Returns true if the value changed this frame.
bool DrawToggle(Rectangle bounds, Color accentColor, bool& value);

// Draw a color swatch square. If selected, shows a highlight border.
// Returns true if clicked this frame.
bool DrawColorSwatch(Rectangle bounds, Color color, bool selected = false);

} // namespace theword::ui

#endif
