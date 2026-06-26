#include "AboutOverlay.h"
#include "core/Theme.h"
#include "Version.h"
#include <string>

namespace theword::renderer {

using namespace theword::core;

AboutOverlay::AboutOverlay(const Font& headingFont, float headingSize, float scale)
    : headingFont(headingFont), headingSize(headingSize), scale(scale),
      active(false) {}

void AboutOverlay::Toggle() { active = !active; }
void AboutOverlay::Dismiss() { active = false; }
bool AboutOverlay::IsActive() const { return active; }

Rectangle AboutOverlay::GetCloseButtonRect() const {
    float sw = (float)GetScreenWidth();
    float sh = (float)GetScreenHeight();
    float dx = (sw - ABOUT_WIDTH * scale) / 2.0f;
    float dy = (sh - ABOUT_HEIGHT * scale) / 2.0f;
    float cx = dx + ABOUT_WIDTH * scale - PADDING * scale - CLOSE_BUTTON_SIZE * scale;
    float cy = dy + PADDING * scale;
    return {cx, cy, CLOSE_BUTTON_SIZE * scale, CLOSE_BUTTON_SIZE * scale};
}

void AboutOverlay::DrawCloseButton() const {
    Rectangle r = GetCloseButtonRect();
    DrawRectangle(r.x, r.y, r.width, r.height, theme::PANEL_BG);
    DrawRectangleLines(r.x, r.y, r.width, r.height, theme::PANEL_BORDER);
    Vector2 textSize = MeasureTextEx(headingFont, "X", headingSize, 1);
    DrawTextEx(headingFont, "X",
               {r.x + (r.width - textSize.x) / 2.0f, r.y + (r.height - textSize.y) / 2.0f},
               headingSize, 1, theme::UI_TEXT);
}

void AboutOverlay::DrawBackdrop() const {
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(),
                  theme::OVERLAY_BG);
}

void AboutOverlay::Draw() {
    if (!active) return;

    DrawBackdrop();

    float sw = (float)GetScreenWidth();
    float sh = (float)GetScreenHeight();
    float actualW = ABOUT_WIDTH * scale;
    float actualH = ABOUT_HEIGHT * scale;
    float dx = (sw - actualW) / 2.0f;
    float dy = (sh - actualH) / 2.0f;

    DrawRectangle(dx, dy, actualW, actualH, theme::PANEL_BG);
    DrawRectangleLines(dx, dy, actualW, actualH, theme::PANEL_BORDER);
    DrawCloseButton();

    float contentX = dx + PADDING * scale;
    float y = dy + PADDING * scale + CLOSE_BUTTON_SIZE * scale + 4;

    std::string versionLabel = std::string("TheWord Bible Study v") + theword::core::APP_VERSION;
    DrawTextEx(headingFont, versionLabel.c_str(),
               {contentX, y}, headingSize, 1, theme::UI_TEXT);
    y += 28 * scale;
    DrawTextEx(headingFont, "Built with Raylib & C++17",
               {contentX, y}, headingSize, 1, theme::UI_TEXT);
    y += 28 * scale;
    DrawTextEx(headingFont, "Data: USFM (offline) + YouVersion API (online)",
               {contentX, y}, headingSize, 1, theme::UI_TEXT);
}

bool AboutOverlay::HandleClick(Vector2 pos) {
    if (!active) return false;

    if (CheckCollisionPointRec(pos, GetCloseButtonRect())) {
        Dismiss();
        return true;
    }

    float sw = (float)GetScreenWidth();
    float sh = (float)GetScreenHeight();
    float actualW = ABOUT_WIDTH * scale;
    float actualH = ABOUT_HEIGHT * scale;
    float dx = (sw - actualW) / 2.0f;
    float dy = (sh - actualH) / 2.0f;

    Rectangle overlayRect = {dx, dy, actualW, actualH};
    if (!CheckCollisionPointRec(pos, overlayRect)) {
        Dismiss();
        return true;
    }

    return true;
}

} // namespace theword::renderer
