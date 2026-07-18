#include "SettingsScreen.h"
#include "NavigationStack.h"
#include "components.h"
#include "core/Theme.h"
#include "core/Config.h"
#include "core/Locale.h"
#include "event/EventBus.h"
#include "event/Events.h"
#include "persistence/PersistenceManager.h"
#include <algorithm>

namespace theword::ui {

using namespace theword::core;

SettingsScreen::SettingsScreen(const Font& font, float fontSize,
                               NavigationStack& navStack,
                               theword::event::EventBus& eventBus,
                               theword::persistence::PersistenceManager& persistence,
                               const theword::core::UIScale& uiScale,
                               float& currentFontSize, int& currentBibleId, bool& immersiveMode)
    : font_(font), fontSize_(fontSize),
      navStack_(navStack), eventBus_(eventBus),
      persistence_(persistence),
      uiScale_(uiScale), currentFontSize_(currentFontSize), currentBibleId_(currentBibleId), immersiveMode_(immersiveMode),
      tapDetector_(uiScale_.dp(10)) {}

void SettingsScreen::Draw() {
    float screenW = static_cast<float>(GetScreenWidth());

    float headerH = uiScale_.dp(48);
    DrawHeaderBar(font_, fontSize_, Locale::Get("Settings"), true, static_cast<int>(screenW), uiScale_);

    float labelSize = fontSize_ * 0.7f;
    float controlSize = fontSize_ * 0.65f;

    float labelX = uiScale_.dp(16);
    float btnW = uiScale_.dp(36);
    float btnH = uiScale_.dp(30);
    float decX = uiScale_.dp(140);
    float valX = uiScale_.dp(190);
    float incX = uiScale_.dp(240);

    // Font row
    float rowY = headerH + uiScale_.dp(40);
    float rowGap = uiScale_.dp(48);

    DrawTextEx(font_, Locale::Get("Font:"), {labelX, rowY}, labelSize, 1, theme::UI_TEXT);

    bool atMin = currentFontSize_ <= config::FONT_SIZE_MIN;
    bool atMax = currentFontSize_ >= config::FONT_SIZE_MAX;

    DrawButton({decX, rowY, btnW, btnH}, "-", font_, controlSize,
               !atMin, theme::UI_BUTTON_TEXT, theme::BUTTON_BG);

    std::string sizeStr = std::to_string(static_cast<int>(currentFontSize_));
    Vector2 sz = MeasureTextEx(font_, sizeStr.c_str(), labelSize, 1);
    DrawTextEx(font_, sizeStr.c_str(), {valX - sz.x / 2.0f, rowY + uiScale_.dp(4)},
               labelSize, 1, theme::UI_TEXT);

    DrawButton({incX, rowY, btnW, btnH}, "+", font_, controlSize,
               !atMax, theme::UI_BUTTON_TEXT, theme::BUTTON_BG);

    // Version row
    rowY += rowGap;
    DrawTextEx(font_, Locale::Get("Version:"), {labelX, rowY}, labelSize, 1, theme::UI_TEXT);

    float verBtnW = uiScale_.dp(52);
    float verBtnH = uiScale_.dp(30);
    float verBtnGap = uiScale_.dp(4);
    float verStartX = uiScale_.dp(140);

    for (int i = 0; i < config::BIBLE_VERSION_COUNT; ++i) {
        float vx = verStartX + i * (verBtnW + verBtnGap);
        bool active = (currentBibleId_ == config::BIBLE_VERSIONS[i].id);
        DrawButton({vx, rowY, verBtnW, verBtnH}, config::BIBLE_VERSIONS[i].label,
                   font_, controlSize, true,
                   theme::UI_TEXT,
                   active ? theme::SWITCH_ON : theme::SWITCH_OFF);
    }

    // Immersive mode row
    rowY += rowGap;
    DrawTextEx(font_, "Modo Limpo:", {labelX, rowY}, labelSize, 1, theme::UI_TEXT);

    float imBtnW = uiScale_.dp(80);
    float imBtnH = uiScale_.dp(30);
    float imOnX = uiScale_.dp(140);
    float imOffX = uiScale_.dp(230);

    DrawButton({imOnX, rowY, imBtnW, imBtnH}, "ON", font_, controlSize,
               true, theme::UI_TEXT, immersiveMode_ ? theme::SWITCH_ON : theme::SWITCH_OFF);
    DrawButton({imOffX, rowY, imBtnW, imBtnH}, "OFF", font_, controlSize,
               true, theme::UI_TEXT, immersiveMode_ ? theme::SWITCH_OFF : theme::SWITCH_ON);

    Vector2 mouse = GetMousePosition();
    float hitW = uiScale_.dp(56);
    bool overControls = mouse.y >= headerH + uiScale_.dp(40)
        || (mouse.y < headerH && mouse.x < hitW);
    SetMouseCursor(overControls ? MOUSE_CURSOR_POINTING_HAND : MOUSE_CURSOR_DEFAULT);
}

bool SettingsScreen::HandleInput(float /*deltaTime*/) {
    if (IsKeyPressed(key::ESCAPE)) {
        navStack_.Pop();
        return true;
    }

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        tapDetector_.OnPress(GetMousePosition());

    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        Vector2 mousePos;
        auto tr = tapDetector_.OnRelease(GetMousePosition(), mousePos);
        if (tr == TapDetector::Result::Drag) { return false; }
        if (tr == TapDetector::Result::Tap) {

        float headerH = uiScale_.dp(48);
        float backW = uiScale_.dp(56);

        if (mousePos.y < headerH && mousePos.x < backW) {
            navStack_.Pop();
            return true;
        }

        float btnW = uiScale_.dp(36);
        float btnH = uiScale_.dp(30);
        float decX = uiScale_.dp(140);
        float incX = uiScale_.dp(240);

        float rowY = headerH + uiScale_.dp(40);
        float rowGap = uiScale_.dp(48);

        // Font decrease
        Rectangle decRect = {decX, rowY, btnW, btnH};
        if (CheckCollisionPointRec(mousePos, decRect)) {
            ChangeFontSize(-config::FONT_SIZE_STEP);
            return true;
        }

        // Font increase
        Rectangle incRect = {incX, rowY, btnW, btnH};
        if (CheckCollisionPointRec(mousePos, incRect)) {
            ChangeFontSize(config::FONT_SIZE_STEP);
            return true;
        }

        // Version row
        rowY += rowGap;
        float verBtnW = uiScale_.dp(52);
        float verBtnH = uiScale_.dp(30);
        float verBtnGap = uiScale_.dp(4);
        float verStartX = uiScale_.dp(140);

        for (int i = 0; i < config::BIBLE_VERSION_COUNT; ++i) {
            float vx = verStartX + i * (verBtnW + verBtnGap);
            Rectangle verRect = {vx, rowY, verBtnW, verBtnH};
            if (CheckCollisionPointRec(mousePos, verRect)) {
                int newId = config::BIBLE_VERSIONS[i].id;
                if (newId != currentBibleId_) {
                    currentBibleId_ = newId;
                    eventBus_.Emit(theword::event::BibleVersionSwitchEvent{newId});
                }
                return true;
            }
        }

        // Immersive mode row
        rowY += rowGap;
        float imBtnW = uiScale_.dp(80);
        float imBtnH = uiScale_.dp(30);
        float imOnX = uiScale_.dp(140);
        float imOffX = uiScale_.dp(230);
        Rectangle imOnRect = {imOnX, rowY, imBtnW, imBtnH};
        Rectangle imOffRect = {imOffX, rowY, imBtnW, imBtnH};
        if (CheckCollisionPointRec(mousePos, imOnRect)) {
            immersiveMode_ = true;
            persistence_.SetPreference("immersive_mode", "1");
            return true;
        }
        if (CheckCollisionPointRec(mousePos, imOffRect)) {
            immersiveMode_ = false;
            persistence_.SetPreference("immersive_mode", "0");
            return true;
        }
    }
    }

    return false;
}

void SettingsScreen::ChangeFontSize(float delta) {
    float newSize = currentFontSize_ + delta;
    newSize = std::max(config::FONT_SIZE_MIN, std::min(config::FONT_SIZE_MAX, newSize));
    if (newSize != currentFontSize_) {
        currentFontSize_ = newSize;
        persistence_.SetPreference("font_size", std::to_string(static_cast<int>(newSize)));
        eventBus_.Emit(theword::event::FontSizeEvent{newSize * uiScale_.dpiScale, 0.0f});
    }
}

} // namespace theword::ui
