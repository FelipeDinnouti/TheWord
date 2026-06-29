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

} // namespace theword::ui

#endif
