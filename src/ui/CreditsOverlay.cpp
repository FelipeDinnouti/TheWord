#include "CreditsOverlay.h"
#include "NavigationStack.h"
#include "components.h"
#include "core/Theme.h"
#include "core/Config.h"
#include "core/Locale.h"
#include "Version.h"
#include <string>

namespace theword::ui {

using namespace theword::core;

CreditsOverlay::CreditsOverlay(const Font& font, float fontSize, NavigationStack& navStack,
                               const theword::core::UIScale& uiScale)
    : font_(font), fontSize_(fontSize), navStack_(navStack), uiScale_(uiScale),
      showTime_(GetTime()),
      tapDetector_(uiScale_.dp(10)) {}

void CreditsOverlay::Draw() {
    float screenW = static_cast<float>(GetScreenWidth());
    float screenH = static_cast<float>(GetScreenHeight());

    float now = static_cast<float>(GetTime());
    float fadeAlpha;
    if (fadingOut_) {
        float elapsed = now - static_cast<float>(fadeOutStartTime_);
        fadeAlpha = std::max(0.0f, 1.0f - elapsed / FADE_DURATION);
        if (fadeAlpha <= 0.0f) {
            popPending_ = true;
            return;
        }
    } else {
        float elapsed = now - static_cast<float>(showTime_);
        fadeAlpha = std::min(1.0f, elapsed / FADE_DURATION);
    }

    Color overlayColor = theme::OVERLAY_BG;
    overlayColor.a = static_cast<unsigned char>(theme::OVERLAY_BG.a * fadeAlpha);
    DrawRectangle(0, 0, static_cast<int>(screenW), static_cast<int>(screenH), overlayColor);

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

    DrawPanel({panelX, panelY, panelW, contentH},
              Fade(theme::PANEL_BG, fadeAlpha),
              Fade(theme::PANEL_BORDER, fadeAlpha));

    float y = panelY + padding;

    std::string title = std::string("TheWord v") + APP_VERSION;
    DrawTextEx(font_, title.c_str(), {panelX + padding, y}, labelSize, 1,
               Fade(theme::UI_TITLE, fadeAlpha));
    y += labelSize + uiScale_.dp(12);

    DrawTextEx(font_, Locale::Get("Built with Raylib & C++17"), {panelX + padding, y},
               smallSize, 1, Fade(theme::UI_TEXT, fadeAlpha));
    y += smallSize + uiScale_.dp(8);

    DrawTextEx(font_, Locale::Get("Data: USFM (offline) + YouVersion API (online)"),
               {panelX + padding, y}, smallSize, 1, Fade(theme::UI_TEXT, fadeAlpha));
    y += smallSize + uiScale_.dp(8);

    DrawTextEx(font_, Locale::Get("Press Escape or tap outside to close"),
               {panelX + padding, y}, smallSize, 1, Fade(theme::UI_TEXT, fadeAlpha));

    SetMouseCursor(MOUSE_CURSOR_DEFAULT);
}

bool CreditsOverlay::HandleInput(float /*deltaTime*/) {
    if (fadingOut_) {
        if (popPending_) {
            navStack_.Pop();
            return true;
        }
        return true;
    }

    if (IsKeyPressed(key::ESCAPE)) {
        fadingOut_ = true;
        fadeOutStartTime_ = GetTime();
        return true;
    }

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        tapDetector_.OnPress(GetMousePosition());

    Vector2 mousePos;
    auto tr = tapDetector_.OnRelease(GetMousePosition(), mousePos);
    if (tr == TapDetector::Result::Drag) return true;
    if (tr == TapDetector::Result::Tap) {

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
            fadingOut_ = true;
            fadeOutStartTime_ = GetTime();
            return true;
        }
        if (mousePos.x >= panelX + panelW - uiScale_.dp(24) && mousePos.y < panelY + uiScale_.dp(24)) {
            fadingOut_ = true;
            fadeOutStartTime_ = GetTime();
            return true;
        }
    }

    return false;
}

} // namespace theword::ui
