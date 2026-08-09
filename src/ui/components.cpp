#include "components.h"
#include "core/Theme.h"
#include "core/Locale.h"
#include <cctype>
#include <algorithm>

namespace theword::ui {

using namespace theword::core;

static float RadiusToRoundness(float radius, float w, float h) {
    float halfMin = std::min(w, h) * 0.5f;
    if (halfMin <= 0.0f) return 0.0f;
    return std::min(radius / halfMin, 1.0f);
}

void DrawHeaderBar(const theword::renderer::DrawContext& ctx, const Font& font, float fontSize,
                   const char* title, bool hasBack) {
    const auto& palette = ctx.themeManager.Current();
    const auto& uiScale = ctx.uiScale;
    int screenWidth = static_cast<int>(uiScale.screenW);
    float barHeight = uiScale.dp(48);
    float labelSize = fontSize * 0.7f;

    DrawRectangle(0, 0, screenWidth, static_cast<int>(barHeight), palette.windowBg);
    DrawRectangle(0, static_cast<int>(barHeight) - 1, screenWidth, 1, palette.buttonBorder);

    if (hasBack) {
        std::string backLabel = std::string("\xE2\x86\xA9 ") + Locale::Get("Back");
        DrawTextEx(font, backLabel.c_str(), {uiScale.dp(8), (barHeight - labelSize) / 2.0f},
                   labelSize, 1, palette.uiText);
    }

    Vector2 titleSize = MeasureTextEx(font, title, labelSize, 1);
    float titleX = (static_cast<float>(screenWidth) - titleSize.x) / 2.0f;
    float titleY = (barHeight - titleSize.y) / 2.0f;
    DrawTextEx(font, title, {titleX, titleY}, labelSize, 1, palette.uiTitle);
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

bool DrawButton(const theword::renderer::DrawContext& ctx, Rectangle bounds, const char* text,
                const Font& font, float fontSize, bool enabled, Color textColor, Color bgColor) {
    const auto& palette = ctx.themeManager.Current();
    Vector2 mouse = GetMousePosition();
    bool hovered = enabled && CheckCollisionPointRec(mouse, bounds);
    bool pressed = hovered && IsMouseButtonDown(MOUSE_LEFT_BUTTON);

    Color fill;
    Color border;
    if (!enabled) {
        fill = palette.buttonBorderDisabled;
        border = palette.buttonBorderDisabled;
    } else if (pressed) {
        fill = theme::Darken(bgColor, theme::PRESS_DARKEN);
        border = theme::Darken(palette.buttonBorder, theme::PRESS_DARKEN);
    } else if (hovered) {
        fill = theme::Darken(bgColor, theme::HOVER_DARKEN);
        border = palette.buttonBorder;
    } else {
        fill = bgColor;
        border = palette.buttonBorder;
    }

    float const bw = 1.0f;
    float roundness = RadiusToRoundness(theme::PANEL_ROUNDING, bounds.width, bounds.height);
    DrawRectangleRounded(bounds, roundness, 8, border);
    Rectangle inner = {bounds.x + bw, bounds.y + bw,
                       bounds.width - bw * 2, bounds.height - bw * 2};
    float innerRoundness = RadiusToRoundness(theme::PANEL_ROUNDING, inner.width, inner.height);
    DrawRectangleRounded(inner, innerRoundness, 8, fill);

    Vector2 textSize = MeasureTextEx(font, text, fontSize, 1);
    float tx = bounds.x + (bounds.width - textSize.x) * 0.5f;
    float ty = bounds.y + (bounds.height - textSize.y) * 0.5f;
    DrawTextEx(font, text, {tx, ty}, fontSize, 1,
               enabled ? textColor : palette.uiButtonTextDisabled);

    return hovered && IsMouseButtonReleased(MOUSE_LEFT_BUTTON);
}

void DrawPanel(Rectangle bounds, Color bgColor, Color borderColor,
               float rounding, float borderThick) {
    float roundness = RadiusToRoundness(rounding, bounds.width, bounds.height);
    DrawRectangleRounded(bounds, roundness, 8, borderColor);
    Rectangle inner = {bounds.x + borderThick, bounds.y + borderThick,
                       bounds.width - borderThick * 2, bounds.height - borderThick * 2};
    float innerRoundness = RadiusToRoundness(rounding, inner.width, inner.height);
    DrawRectangleRounded(inner, innerRoundness, 8, bgColor);
}

bool DrawTextItem(const theword::renderer::DrawContext& ctx, Rectangle bounds, const char* text,
                  const Font& font, float fontSize, bool selected, Color textColor,
                  Color selTextColor) {
    const auto& palette = ctx.themeManager.Current();
    Vector2 mouse = GetMousePosition();
    bool hovered = CheckCollisionPointRec(mouse, bounds);
    bool pressed = hovered && IsMouseButtonDown(MOUSE_LEFT_BUTTON);

    Color bg;
    if (selected && pressed) {
        bg = theme::Darken(palette.selectedBg, theme::PRESS_DARKEN);
    } else if (selected) {
        bg = palette.selectedBg;
    } else if (pressed) {
        bg = theme::Darken(palette.windowBg, theme::PRESS_DARKEN);
    } else if (hovered) {
        bg = theme::Darken(palette.windowBg, theme::HOVER_DARKEN);
    } else {
        bg = {0, 0, 0, 0};
    }

    if (bg.a > 0) {
        float roundness = RadiusToRoundness(theme::PANEL_ROUNDING, bounds.width, bounds.height);
        DrawRectangleRounded(bounds, roundness, 8, bg);
    }

    Color c = selected ? selTextColor : textColor;
    float padding = 12.0f;
    DrawTextEx(font, text, {bounds.x + padding, bounds.y + (bounds.height - fontSize) * 0.5f},
               fontSize, 1, c);

    return hovered && IsMouseButtonReleased(MOUSE_LEFT_BUTTON);
}

bool DrawToggle(const theword::renderer::DrawContext& ctx, Rectangle bounds,
                Color accentColor, bool& value) {
    const auto& palette = ctx.themeManager.Current();
    Vector2 mouse = GetMousePosition();
    bool hovered = CheckCollisionPointRec(mouse, bounds);
    bool clicked = hovered && IsMouseButtonReleased(MOUSE_LEFT_BUTTON);

    if (clicked) {
        value = !value;
    }

    float roundness = RadiusToRoundness(bounds.height * 0.5f, bounds.width, bounds.height);
    Color trackColor = value ? accentColor : palette.switchOff;
    DrawRectangleRounded(bounds, roundness, 8, trackColor);

    float knobSize = bounds.height - 4.0f;
    float knobX = value ? bounds.x + bounds.width - knobSize - 2.0f : bounds.x + 2.0f;
    float knobY = bounds.y + 2.0f;
    DrawRectangleRounded({knobX, knobY, knobSize, knobSize}, 1.0f, 8, WHITE);
    DrawRectangleRoundedLines({knobX, knobY, knobSize, knobSize}, 1.0f, 8, 0.5f, GRAY);

    return clicked;
}

bool DrawColorSwatch(const theword::renderer::DrawContext& ctx, Rectangle bounds, Color color,
                     bool selected) {
    const auto& palette = ctx.themeManager.Current();
    Vector2 mouse = GetMousePosition();
    bool hovered = CheckCollisionPointRec(mouse, bounds);
    bool clicked = hovered && IsMouseButtonReleased(MOUSE_LEFT_BUTTON);

    Color fill = color;
    if (hovered && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
        fill = theme::Darken(color, theme::PRESS_DARKEN);
    } else if (hovered) {
        fill = theme::Darken(color, theme::HOVER_DARKEN);
    }

    float borderW = selected ? 2.0f : 1.0f;
    Color borderC = selected ? BLACK : palette.buttonBorder;

    float roundness = RadiusToRoundness(3.0f, bounds.width, bounds.height);
    DrawRectangleRounded(bounds, roundness, 8, borderC);
    Rectangle inner = {bounds.x + borderW, bounds.y + borderW,
                       bounds.width - borderW * 2, bounds.height - borderW * 2};
    float innerRoundness = RadiusToRoundness(3.0f, inner.width, inner.height);
    DrawRectangleRounded(inner, innerRoundness, 8, fill);

    return clicked;
}

} // namespace theword::ui
