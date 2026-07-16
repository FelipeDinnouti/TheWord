#include "SettingsScreen.h"
#include "NavigationStack.h"
#include "components.h"
#include "core/Theme.h"
#include "core/Config.h"
#include "core/Locale.h"
#include "event/EventBus.h"
#include "event/Events.h"
#include "highlight/Highlighter.h"
#include "persistence/PersistenceManager.h"
#include <algorithm>

namespace theword::ui {

using namespace theword::core;

SettingsScreen::SettingsScreen(const Font& font, float fontSize,
                               NavigationStack& navStack,
                               theword::event::EventBus& eventBus,
                               theword::highlight::Highlighter& highlighter,
                               theword::persistence::PersistenceManager& persistence,
                               const theword::core::UIScale& uiScale,
                                float& currentFontSize, bool& versionOnline, bool& immersiveMode)
    : font_(font), fontSize_(fontSize),
      navStack_(navStack), eventBus_(eventBus),
      highlighter_(highlighter), persistence_(persistence),
      uiScale_(uiScale), currentFontSize_(currentFontSize), versionOnline_(versionOnline), immersiveMode_(immersiveMode),
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
    float swatchSize = uiScale_.dp(28);
    float swatchGap = uiScale_.dp(8);
    float colorStartX = uiScale_.dp(140);

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

    // Source row
    rowY += rowGap;
    DrawTextEx(font_, Locale::Get("Source:"), {labelX, rowY}, labelSize, 1, theme::UI_TEXT);

    float srcBtnW = uiScale_.dp(80);
    float srcBtnH = uiScale_.dp(30);
    float usfmX = uiScale_.dp(140);
    float onlineX = uiScale_.dp(230);

    DrawButton({usfmX, rowY, srcBtnW, srcBtnH}, "USFM", font_, controlSize,
               true, theme::UI_TEXT, versionOnline_ ? theme::SWITCH_OFF : theme::SWITCH_ON);
    DrawButton({onlineX, rowY, srcBtnW, srcBtnH}, "API", font_, controlSize,
               true, theme::UI_TEXT, versionOnline_ ? theme::SWITCH_ON : theme::SWITCH_OFF);

    // Color row
    rowY += rowGap;
    DrawTextEx(font_, Locale::Get("Color:"), {labelX, rowY}, labelSize, 1, theme::UI_TEXT);

    const auto& types = highlighter_.GetTypes();
    int activeId = highlighter_.GetActiveTypeId();
    for (size_t i = 0; i < types.size(); ++i) {
        float swatchX = colorStartX + i * (swatchSize + swatchGap);
        Color c = {types[i].color.r, types[i].color.g, types[i].color.b, 255};
        DrawColorSwatch({swatchX, rowY, swatchSize, swatchSize}, c, types[i].id == activeId);
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

        // Back button
        if (mousePos.y < headerH && mousePos.x < backW) {
            navStack_.Pop();
            return true;
        }

        float btnW = uiScale_.dp(36);
        float btnH = uiScale_.dp(30);
        float decX = uiScale_.dp(140);
        float incX = uiScale_.dp(240);
        float srcBtnW = uiScale_.dp(80);
        float srcBtnH = uiScale_.dp(30);
        float usfmX = uiScale_.dp(140);
        float onlineX = uiScale_.dp(230);
        float swatchSize = uiScale_.dp(28);
        float swatchGap = uiScale_.dp(8);
        float colorStartX = uiScale_.dp(140);

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

        // Source row
        rowY += rowGap;
        Rectangle usfmRect = {usfmX, rowY, srcBtnW, srcBtnH};
        if (CheckCollisionPointRec(mousePos, usfmRect)) {
            SwitchSource(false);
            return true;
        }
        Rectangle apiRect = {onlineX, rowY, srcBtnW, srcBtnH};
        if (CheckCollisionPointRec(mousePos, apiRect)) {
            SwitchSource(true);
            return true;
        }

        // Color row
        rowY += rowGap;
        const auto& types = highlighter_.GetTypes();
        for (size_t i = 0; i < types.size(); ++i) {
            float swatchX = colorStartX + i * (swatchSize + swatchGap);
            Rectangle swatchRect = {swatchX, rowY, swatchSize, swatchSize};
            if (CheckCollisionPointRec(mousePos, swatchRect)) {
                highlighter_.SetActiveTypeId(types[i].id);
                persistence_.SetPreference("active_color", std::to_string(types[i].id));
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

void SettingsScreen::SwitchSource(bool online) {
    if (online == versionOnline_) return;
    versionOnline_ = online;
    persistence_.SetPreference("active_version", online ? "online" : "offline");
    eventBus_.Emit(theword::event::SourceSwitchEvent{online});
}

} // namespace theword::ui
