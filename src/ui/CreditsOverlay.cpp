#include "CreditsOverlay.h"
#include "NavigationStack.h"
#include "core/Theme.h"
#include "core/Config.h"
#include "Version.h"
#include <string>

namespace theword::ui {

using namespace theword::core;

CreditsOverlay::CreditsOverlay(const Font& font, float fontSize, NavigationStack& navStack)
    : font_(font), fontSize_(fontSize), navStack_(navStack) {}

void CreditsOverlay::Draw() {
    float screenW = static_cast<float>(GetScreenWidth());
    float screenH = static_cast<float>(GetScreenHeight());

    DrawRectangle(0, 0, static_cast<int>(screenW), static_cast<int>(screenH), theme::OVERLAY_BG);

    float panelX = (screenW - PANEL_WIDTH) / 2.0f;
    float panelY = (screenH - PANEL_HEIGHT) / 2.0f;

    DrawRectangle(static_cast<int>(panelX), static_cast<int>(panelY),
                  static_cast<int>(PANEL_WIDTH), static_cast<int>(PANEL_HEIGHT), theme::PANEL_BG);
    DrawRectangleLines(static_cast<int>(panelX), static_cast<int>(panelY),
                       static_cast<int>(PANEL_WIDTH), static_cast<int>(PANEL_HEIGHT),
                       theme::PANEL_BORDER);

    float labelSize = fontSize_ * 0.7f;
    float smallSize = fontSize_ * 0.55f;
    float y = panelY + PADDING;

    std::string title = std::string("TheWord v") + APP_VERSION;
    DrawTextEx(font_, title.c_str(), {panelX + PADDING, y}, labelSize, 1, theme::UI_TITLE);
    y += labelSize + 12.0f;

    DrawTextEx(font_, "Built with Raylib & C++17", {panelX + PADDING, y},
               smallSize, 1, theme::UI_TEXT);
    y += smallSize + 8.0f;

    DrawTextEx(font_, "Data: USFM (offline) + YouVersion API (online)",
               {panelX + PADDING, y}, smallSize, 1, theme::UI_TEXT);
    y += smallSize + 8.0f;

    DrawTextEx(font_, "Press Escape or tap outside to close",
               {panelX + PADDING, y}, smallSize, 1, theme::UI_TEXT);
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
        float panelX = (screenW - PANEL_WIDTH) / 2.0f;
        float panelY = (screenH - PANEL_HEIGHT) / 2.0f;
        Rectangle panelRect = {panelX, panelY, PANEL_WIDTH, PANEL_HEIGHT};

        if (!CheckCollisionPointRec(mousePos, panelRect)) {
            navStack_.Pop();
            return true;
        }
        // X close button area approx top-right
        if (mousePos.x >= panelX + PANEL_WIDTH - 40.0f && mousePos.y < panelY + 40.0f) {
            navStack_.Pop();
            return true;
        }
    }

    return false;
}

} // namespace theword::ui
