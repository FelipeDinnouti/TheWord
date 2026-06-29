#include "SettingsScreen.h"
#include "NavigationStack.h"
#include "components.h"
#include "core/Theme.h"
#include "core/Config.h"
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
                               float& currentFontSize, bool& versionOnline)
    : font_(font), fontSize_(fontSize),
      navStack_(navStack), eventBus_(eventBus),
      highlighter_(highlighter), persistence_(persistence),
      uiScale_(uiScale), currentFontSize_(currentFontSize), versionOnline_(versionOnline) {}

void SettingsScreen::Draw() {
    float screenW = static_cast<float>(GetScreenWidth());

    float headerH = uiScale_.dp(48);
    DrawHeaderBar(font_, fontSize_, "Settings", true, static_cast<int>(screenW), uiScale_);

    float labelSize = fontSize_ * 0.7f;
    float controlSize = fontSize_ * 0.65f;

    float labelX = uiScale_.dp(16);
    float btnW = uiScale_.dp(36);
    float btnH = uiScale_.dp(30);
    float decX = uiScale_.dp(140);
    float valX = uiScale_.dp(190);
    float incX = uiScale_.dp(240);
    float srcBtnW = uiScale_.dp(80);
    float srcBtnH = uiScale_.dp(30);
    float usfmX = uiScale_.dp(140);
    float onlineX = uiScale_.dp(230);
    float swatchSize = uiScale_.dp(28);
    float swatchGap = uiScale_.dp(8);
    float colorStartX = uiScale_.dp(140);

    // Font row
    float rowY = headerH + uiScale_.dp(40);
    float rowGap = uiScale_.dp(48);
    DrawTextEx(font_, "Font:", {labelX, rowY}, labelSize, 1, theme::UI_TEXT);

    // Minus button
    bool atMin = currentFontSize_ <= config::FONT_SIZE_MIN;
    DrawRectangle(static_cast<int>(decX), static_cast<int>(rowY),
                  static_cast<int>(btnW), static_cast<int>(btnH),
                  theme::BUTTON_BG);
    DrawRectangleLines(static_cast<int>(decX), static_cast<int>(rowY),
                       static_cast<int>(btnW), static_cast<int>(btnH),
                       atMin ? theme::BUTTON_BORDER_DISABLED : theme::BUTTON_BORDER);
    DrawTextEx(font_, "-", {decX + uiScale_.dp(10), rowY + uiScale_.dp(2)}, controlSize, 1,
               atMin ? theme::UI_BUTTON_TEXT_DISABLED : theme::UI_BUTTON_TEXT);

    // Size value
    std::string sizeStr = std::to_string(static_cast<int>(currentFontSize_));
    Vector2 sz = MeasureTextEx(font_, sizeStr.c_str(), labelSize, 1);
    DrawTextEx(font_, sizeStr.c_str(), {valX - sz.x / 2.0f, rowY + uiScale_.dp(4)},
               labelSize, 1, theme::UI_TEXT);

    // Plus button
    bool atMax = currentFontSize_ >= config::FONT_SIZE_MAX;
    DrawRectangle(static_cast<int>(incX), static_cast<int>(rowY),
                  static_cast<int>(btnW), static_cast<int>(btnH),
                  theme::BUTTON_BG);
    DrawRectangleLines(static_cast<int>(incX), static_cast<int>(rowY),
                       static_cast<int>(btnW), static_cast<int>(btnH),
                       atMax ? theme::BUTTON_BORDER_DISABLED : theme::BUTTON_BORDER);
    DrawTextEx(font_, "+", {incX + uiScale_.dp(10), rowY + uiScale_.dp(2)}, controlSize, 1,
               atMax ? theme::UI_BUTTON_TEXT_DISABLED : theme::UI_BUTTON_TEXT);

    // Source row
    rowY += rowGap;
    DrawTextEx(font_, "Source:", {labelX, rowY}, labelSize, 1, theme::UI_TEXT);

    // USFM button
    DrawRectangle(static_cast<int>(usfmX), static_cast<int>(rowY),
                  static_cast<int>(srcBtnW), static_cast<int>(srcBtnH),
                  versionOnline_ ? theme::SWITCH_OFF : theme::SWITCH_ON);
    DrawRectangleLines(static_cast<int>(usfmX), static_cast<int>(rowY),
                       static_cast<int>(srcBtnW), static_cast<int>(srcBtnH),
                       theme::BUTTON_BORDER);
    DrawTextEx(font_, "USFM", {usfmX + uiScale_.dp(20), rowY + uiScale_.dp(4)}, controlSize, 1, theme::UI_TEXT);

    // API button
    DrawRectangle(static_cast<int>(onlineX), static_cast<int>(rowY),
                  static_cast<int>(srcBtnW), static_cast<int>(srcBtnH),
                  versionOnline_ ? theme::SWITCH_ON : theme::SWITCH_OFF);
    DrawRectangleLines(static_cast<int>(onlineX), static_cast<int>(rowY),
                       static_cast<int>(srcBtnW), static_cast<int>(srcBtnH),
                       theme::BUTTON_BORDER);
    DrawTextEx(font_, "API", {onlineX + uiScale_.dp(25), rowY + uiScale_.dp(4)}, controlSize, 1, theme::UI_TEXT);

    // Color row
    rowY += rowGap;
    DrawTextEx(font_, "Color:", {labelX, rowY}, labelSize, 1, theme::UI_TEXT);

    const auto& types = highlighter_.GetTypes();
    int activeId = highlighter_.GetActiveTypeId();
    for (size_t i = 0; i < types.size(); ++i) {
        float swatchX = colorStartX + i * (swatchSize + swatchGap);
        Color c = {types[i].color.r, types[i].color.g, types[i].color.b, 255};
        DrawRectangle(static_cast<int>(swatchX), static_cast<int>(rowY),
                      static_cast<int>(swatchSize), static_cast<int>(swatchSize), c);
        DrawRectangleLines(static_cast<int>(swatchX) - 1, static_cast<int>(rowY) - 1,
                           static_cast<int>(swatchSize) + 2, static_cast<int>(swatchSize) + 2,
                           types[i].id == activeId ? theme::PANEL_BORDER : theme::BUTTON_BORDER);
    }
}

bool SettingsScreen::HandleInput(float /*deltaTime*/) {
    if (IsKeyPressed(key::ESCAPE)) {
        navStack_.Pop();
        return true;
    }

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 mousePos = GetMousePosition();

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
