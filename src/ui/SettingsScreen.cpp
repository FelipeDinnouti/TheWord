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
                               float scale, float& currentFontSize, bool& versionOnline)
    : font_(font), fontSize_(fontSize),
      navStack_(navStack), eventBus_(eventBus),
      highlighter_(highlighter), persistence_(persistence),
      scale_(scale), currentFontSize_(currentFontSize), versionOnline_(versionOnline) {}

void SettingsScreen::Draw() {
    float screenW = static_cast<float>(GetScreenWidth());

    DrawHeaderBar(font_, fontSize_, "Settings", true, static_cast<int>(screenW));

    float labelSize = fontSize_ * 0.7f;
    float controlSize = fontSize_ * 0.65f;

    // Font row
    float rowY = ROW_Y1;
    DrawTextEx(font_, "Font:", {LABEL_X, rowY}, labelSize, 1, theme::UI_TEXT);

    // Minus button
    bool atMin = currentFontSize_ <= config::FONT_SIZE_MIN;
    DrawRectangle(static_cast<int>(FONT_DEC_X), static_cast<int>(rowY),
                  static_cast<int>(FONT_BTN_W), static_cast<int>(FONT_BTN_H),
                  theme::BUTTON_BG);
    DrawRectangleLines(static_cast<int>(FONT_DEC_X), static_cast<int>(rowY),
                       static_cast<int>(FONT_BTN_W), static_cast<int>(FONT_BTN_H),
                       atMin ? theme::BUTTON_BORDER_DISABLED : theme::BUTTON_BORDER);
    DrawTextEx(font_, "-", {FONT_DEC_X + 10.0f, rowY + 2.0f}, controlSize, 1,
               atMin ? theme::UI_BUTTON_TEXT_DISABLED : theme::UI_BUTTON_TEXT);

    // Size value
    std::string sizeStr = std::to_string(static_cast<int>(currentFontSize_));
    Vector2 sz = MeasureTextEx(font_, sizeStr.c_str(), labelSize, 1);
    DrawTextEx(font_, sizeStr.c_str(), {FONT_VAL_X - sz.x / 2.0f, rowY + 4.0f},
               labelSize, 1, theme::UI_TEXT);

    // Plus button
    bool atMax = currentFontSize_ >= config::FONT_SIZE_MAX;
    DrawRectangle(static_cast<int>(FONT_INC_X), static_cast<int>(rowY),
                  static_cast<int>(FONT_BTN_W), static_cast<int>(FONT_BTN_H),
                  theme::BUTTON_BG);
    DrawRectangleLines(static_cast<int>(FONT_INC_X), static_cast<int>(rowY),
                       static_cast<int>(FONT_BTN_W), static_cast<int>(FONT_BTN_H),
                       atMax ? theme::BUTTON_BORDER_DISABLED : theme::BUTTON_BORDER);
    DrawTextEx(font_, "+", {FONT_INC_X + 10.0f, rowY + 2.0f}, controlSize, 1,
               atMax ? theme::UI_BUTTON_TEXT_DISABLED : theme::UI_BUTTON_TEXT);

    // Source row
    rowY += ROW_GAP;
    DrawTextEx(font_, "Source:", {LABEL_X, rowY}, labelSize, 1, theme::UI_TEXT);

    // USFM button
    DrawRectangle(static_cast<int>(SRC_USFM_X), static_cast<int>(rowY),
                  static_cast<int>(SRC_BTN_W), static_cast<int>(SRC_BTN_H),
                  versionOnline_ ? theme::SWITCH_OFF : theme::SWITCH_ON);
    DrawRectangleLines(static_cast<int>(SRC_USFM_X), static_cast<int>(rowY),
                       static_cast<int>(SRC_BTN_W), static_cast<int>(SRC_BTN_H),
                       theme::BUTTON_BORDER);
    DrawTextEx(font_, "USFM", {SRC_USFM_X + 20.0f, rowY + 4.0f}, controlSize, 1, theme::UI_TEXT);

    // API button
    DrawRectangle(static_cast<int>(SRC_ONLINE_X), static_cast<int>(rowY),
                  static_cast<int>(SRC_BTN_W), static_cast<int>(SRC_BTN_H),
                  versionOnline_ ? theme::SWITCH_ON : theme::SWITCH_OFF);
    DrawRectangleLines(static_cast<int>(SRC_ONLINE_X), static_cast<int>(rowY),
                       static_cast<int>(SRC_BTN_W), static_cast<int>(SRC_BTN_H),
                       theme::BUTTON_BORDER);
    DrawTextEx(font_, "API", {SRC_ONLINE_X + 25.0f, rowY + 4.0f}, controlSize, 1, theme::UI_TEXT);

    // Color row
    rowY += ROW_GAP;
    DrawTextEx(font_, "Color:", {LABEL_X, rowY}, labelSize, 1, theme::UI_TEXT);

    const auto& types = highlighter_.GetTypes();
    int activeId = highlighter_.GetActiveTypeId();
    for (size_t i = 0; i < types.size(); ++i) {
        float swatchX = COLOR_START_X + i * (SWATCH_SIZE + SWATCH_GAP);
        Color c = {types[i].color.r, types[i].color.g, types[i].color.b, 255};
        DrawRectangle(static_cast<int>(swatchX), static_cast<int>(rowY),
                      static_cast<int>(SWATCH_SIZE), static_cast<int>(SWATCH_SIZE), c);
        DrawRectangleLines(static_cast<int>(swatchX) - 1, static_cast<int>(rowY) - 1,
                           static_cast<int>(SWATCH_SIZE) + 2, static_cast<int>(SWATCH_SIZE) + 2,
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

        // Back button
        if (mousePos.y < HEADER_HEIGHT && mousePos.x < BACK_AREA_WIDTH) {
            navStack_.Pop();
            return true;
        }

        float rowY = ROW_Y1;

        // Font decrease
        Rectangle decRect = {FONT_DEC_X, rowY, FONT_BTN_W, FONT_BTN_H};
        if (CheckCollisionPointRec(mousePos, decRect)) {
            ChangeFontSize(-config::FONT_SIZE_STEP);
            return true;
        }

        // Font increase
        Rectangle incRect = {FONT_INC_X, rowY, FONT_BTN_W, FONT_BTN_H};
        if (CheckCollisionPointRec(mousePos, incRect)) {
            ChangeFontSize(config::FONT_SIZE_STEP);
            return true;
        }

        // Source row
        rowY += ROW_GAP;
        Rectangle usfmRect = {SRC_USFM_X, rowY, SRC_BTN_W, SRC_BTN_H};
        if (CheckCollisionPointRec(mousePos, usfmRect)) {
            SwitchSource(false);
            return true;
        }
        Rectangle apiRect = {SRC_ONLINE_X, rowY, SRC_BTN_W, SRC_BTN_H};
        if (CheckCollisionPointRec(mousePos, apiRect)) {
            SwitchSource(true);
            return true;
        }

        // Color row
        rowY += ROW_GAP;
        const auto& types = highlighter_.GetTypes();
        for (size_t i = 0; i < types.size(); ++i) {
            float swatchX = COLOR_START_X + i * (SWATCH_SIZE + SWATCH_GAP);
            Rectangle swatchRect = {swatchX, rowY, SWATCH_SIZE, SWATCH_SIZE};
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
        eventBus_.Emit(theword::event::FontSizeEvent{newSize * scale_, 0.0f});
    }
}

void SettingsScreen::SwitchSource(bool online) {
    if (online == versionOnline_) return;
    versionOnline_ = online;
    persistence_.SetPreference("active_version", online ? "online" : "offline");
    eventBus_.Emit(theword::event::SourceSwitchEvent{online});
}

} // namespace theword::ui
