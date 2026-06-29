#include "CreditsOverlay.h"
#include "NavigationStack.h"
#include "core/Theme.h"
#include "core/Config.h"
#include "Version.h"
#include <string>

namespace theword::ui {

using namespace theword::core;

CreditsOverlay::CreditsOverlay(const Font& font, float fontSize, NavigationStack& navStack,
                               const theword::core::UIScale& uiScale)
    : font_(font), fontSize_(fontSize), navStack_(navStack), uiScale_(uiScale) {}

void CreditsOverlay::Draw() {
    float screenW = static_cast<float>(GetScreenWidth());
    float screenH = static_cast<float>(GetScreenHeight());

    DrawRectangle(0, 0, static_cast<int>(screenW), static_cast<int>(screenH), theme::OVERLAY_BG);

    float panelW = uiScale_.fitScreen(90, 400);
    float padding = uiScale_.dp(16);
    float panelX = (screenW - panelW) / 2.0f;

    float labelSize = fontSize_ * 0.7f;
    float smallSize = fontSize_ * 0.55f;

    float contentH = padding
        + labelSize + uiScale_.dp(12)
        + smallSize + uiScale_.dp(8)
        + smallSize + uiScale_.dp(8)
        + smallSize + padding;
    float panelY = (screenH - contentH) / 2.0f;

    DrawRectangle(static_cast<int>(panelX), static_cast<int>(panelY),
                  static_cast<int>(panelW), static_cast<int>(contentH), theme::PANEL_BG);
    DrawRectangleLines(static_cast<int>(panelX), static_cast<int>(panelY),
                       static_cast<int>(panelW), static_cast<int>(contentH),
                       theme::PANEL_BORDER);

    float y = panelY + padding;

    std::string title = std::string("TheWord v") + APP_VERSION;
    DrawTextEx(font_, title.c_str(), {panelX + padding, y}, labelSize, 1, theme::UI_TITLE);
    y += labelSize + uiScale_.dp(12);

    DrawTextEx(font_, "Built with Raylib & C++17", {panelX + padding, y},
               smallSize, 1, theme::UI_TEXT);
    y += smallSize + uiScale_.dp(8);

    DrawTextEx(font_, "Data: USFM (offline) + YouVersion API (online)",
               {panelX + padding, y}, smallSize, 1, theme::UI_TEXT);
    y += smallSize + uiScale_.dp(8);

    DrawTextEx(font_, "Press Escape or tap outside to close",
               {panelX + padding, y}, smallSize, 1, theme::UI_TEXT);
}

bool CreditsOverlay::HandleInput(float /*deltaTime*/) {
    if (IsKeyPressed(key::ESCAPE)) {
        navStack_.Pop();
        return true;
    }

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 mousePos = GetMousePosition();
        float screenW = static_cast<float>(GetScreenWidth());
        float screenH = static_cast<float>(GetScreenHeight());
        float panelW = uiScale_.fitScreen(90, 400);
        float panelX = (screenW - panelW) / 2.0f;
        float padding = uiScale_.dp(16);
        float contentH = padding
            + (fontSize_ * 0.7f) + uiScale_.dp(12)
            + 3 * (fontSize_ * 0.55f) + 2 * uiScale_.dp(8) + padding;
        float panelY = (screenH - contentH) / 2.0f;
        Rectangle panelRect = {panelX, panelY, panelW, contentH};

        if (!CheckCollisionPointRec(mousePos, panelRect)) {
            navStack_.Pop();
            return true;
        }
        if (mousePos.x >= panelX + panelW - uiScale_.dp(24) && mousePos.y < panelY + uiScale_.dp(24)) {
            navStack_.Pop();
            return true;
        }
    }

    return false;
}

} // namespace theword::ui
