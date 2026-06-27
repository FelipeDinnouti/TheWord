#include "components.h"
#include "core/Theme.h"
#include <cctype>

namespace theword::ui {

using namespace theword::core;

void DrawHeaderBar(const Font& font, float fontSize, const char* title,
                   bool hasBack, int screenWidth) {
    float barHeight = 60.0f;
    float labelSize = fontSize * 0.7f;

    DrawRectangle(0, 0, screenWidth, static_cast<int>(barHeight), theme::WINDOW_BG);
    DrawRectangle(0, static_cast<int>(barHeight) - 1, screenWidth, 1, LIGHTGRAY);

    if (hasBack) {
        DrawTextEx(font, "\xE2\x86\xA9 Back", {12.0f, (barHeight - labelSize) / 2.0f},
                   labelSize, 1, theme::UI_TEXT);
    }

    Vector2 titleSize = MeasureTextEx(font, title, labelSize, 1);
    float titleX = (static_cast<float>(screenWidth) - titleSize.x) / 2.0f;
    float titleY = (barHeight - titleSize.y) / 2.0f;
    DrawTextEx(font, title, {titleX, titleY}, labelSize, 1, theme::UI_TITLE);
}

bool StartsWithIgnoreCase(const std::string& str, const std::string& prefix) {
    if (str.size() < prefix.size()) return false;
    for (size_t i = 0; i < prefix.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(str[i])) !=
            std::tolower(static_cast<unsigned char>(prefix[i])))
            return false;
    }
    return true;
}

} // namespace theword::ui
