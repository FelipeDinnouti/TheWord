#include "SettingsPanel.h"
#include "highlight/Highlighter.h"
#include "core/Theme.h"
#include "core/Config.h"

namespace theword::renderer {

using namespace theword::core;

SettingsPanel::SettingsPanel(const Font& headingFont, float headingSize,
                             theword::highlight::Highlighter& highlighter, float scale)
    : headingFont(headingFont), headingSize(headingSize),
      highlighter(highlighter), scale(scale), active(false) {}

void SettingsPanel::Toggle() { active = !active; }
void SettingsPanel::Dismiss() { active = false; }
bool SettingsPanel::IsActive() const { return active; }

Rectangle SettingsPanel::GetPanelRect() const {
    float sw = (float)GetScreenWidth();
    float sh = (float)GetScreenHeight();
    float w = SETTINGS_WIDTH * scale;
    float h = SETTINGS_HEIGHT * scale;
    return {(sw - w) / 2.0f, (sh - h) / 2.0f, w, h};
}

Rectangle SettingsPanel::GetCloseButtonRect(Rectangle panel) const {
    return {panel.x + panel.width - CLOSE_SIZE * scale - CLOSE_MARGIN * scale,
            panel.y + CLOSE_MARGIN * scale,
            CLOSE_SIZE * scale, CLOSE_SIZE * scale};
}

void SettingsPanel::DrawBackdrop() const {
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), theme::OVERLAY_BG);
}

void SettingsPanel::DrawCloseButton(Rectangle panel) const {
    Rectangle closeBtn = GetCloseButtonRect(panel);
    DrawRectangleRec(closeBtn, theme::BUTTON_BG);
    DrawRectangleLinesEx(closeBtn, 1, theme::BUTTON_BORDER);
    Vector2 closeLabelSize = MeasureTextEx(headingFont, "X", headingSize * theme::FONT_DETAIL, 1);
    float closeTextX = closeBtn.x + (closeBtn.width - closeLabelSize.x) / 2.0f;
    DrawTextEx(headingFont, "X", {closeTextX, closeBtn.y + 1},
               headingSize * theme::FONT_DETAIL, 1, theme::UI_TEXT);
}

void SettingsPanel::Draw(float currentFontSize, bool versionOnline) {
    if (!active) return;

    DrawBackdrop();
    Rectangle panel = GetPanelRect();

    DrawRectangleRec(panel, theme::PANEL_BG);
    DrawRectangleLinesEx(panel, 1, theme::PANEL_BORDER);

    DrawTextEx(headingFont, "Settings", {panel.x + SETTINGS_LABEL_X * scale,
               panel.y + SETTINGS_LABEL_X * scale}, headingSize, 1, theme::UI_TITLE);
    DrawCloseButton(panel);

    // Font row
    float rowY = panel.y + SETTINGS_ROW1_Y * scale;
    DrawTextEx(headingFont, "Font:", {panel.x + SETTINGS_LABEL_X * scale, rowY},
               headingSize * theme::FONT_LABEL, 1, theme::UI_TEXT);

    float btnX = panel.x + FONT_BTN_X * scale;
    DrawRectangle(btnX, rowY, FONT_BTN_W * scale, FONT_BTN_H * scale, theme::BUTTON_BG);
    DrawRectangleLines(btnX, rowY, FONT_BTN_W * scale, FONT_BTN_H * scale,
        currentFontSize <= config::FONT_SIZE_MIN ? theme::BUTTON_BORDER_DISABLED : theme::BUTTON_BORDER);
    DrawTextEx(headingFont, "-", {btnX + 8 * scale, rowY + 1 * scale},
               headingSize * theme::FONT_LABEL, 1,
        currentFontSize <= config::FONT_SIZE_MIN ? theme::UI_BUTTON_TEXT_DISABLED : theme::UI_BUTTON_TEXT);

    std::string sizeStr = std::to_string((int)currentFontSize);
    Vector2 sz = MeasureTextEx(headingFont, sizeStr.c_str(), headingSize * theme::FONT_LABEL, 1);
    DrawTextEx(headingFont, sizeStr.c_str(),
               {panel.x + FONT_SIZE_X * scale - sz.x / 2, rowY + 2 * scale},
               headingSize * theme::FONT_LABEL, 1, theme::UI_TEXT);

    float plusX = panel.x + FONT_PLUS_X * scale;
    DrawRectangle(plusX, rowY, FONT_BTN_W * scale, FONT_BTN_H * scale, theme::BUTTON_BG);
    DrawRectangleLines(plusX, rowY, FONT_BTN_W * scale, FONT_BTN_H * scale,
        currentFontSize >= config::FONT_SIZE_MAX ? theme::BUTTON_BORDER_DISABLED : theme::BUTTON_BORDER);
    DrawTextEx(headingFont, "+", {plusX + 8 * scale, rowY + 1 * scale},
               headingSize * theme::FONT_LABEL, 1,
        currentFontSize >= config::FONT_SIZE_MAX ? theme::UI_BUTTON_TEXT_DISABLED : theme::UI_BUTTON_TEXT);

    // Source row
    rowY += SETTINGS_ROW_GAP * scale;
    DrawTextEx(headingFont, "Source:", {panel.x + SETTINGS_LABEL_X * scale, rowY},
               headingSize * theme::FONT_LABEL, 1, theme::UI_TEXT);

    float usfmX = panel.x + SRC_USFM_X * scale;
    DrawRectangle(usfmX, rowY, SRC_BTN_W * scale, SRC_BTN_H * scale,
                  versionOnline ? theme::SWITCH_OFF : theme::SWITCH_ON);
    DrawRectangleLines(usfmX, rowY, SRC_BTN_W * scale, SRC_BTN_H * scale, theme::BUTTON_BORDER);
    DrawTextEx(headingFont, "USFM", {usfmX + 10 * scale, rowY + 2 * scale},
               headingSize * theme::FONT_DETAIL, 1, theme::UI_TEXT);

    float apiX = panel.x + SRC_ONLINE_X * scale;
    DrawRectangle(apiX, rowY, SRC_BTN_W * scale, SRC_BTN_H * scale,
                  versionOnline ? theme::SWITCH_ON : theme::SWITCH_OFF);
    DrawRectangleLines(apiX, rowY, SRC_BTN_W * scale, SRC_BTN_H * scale, theme::BUTTON_BORDER);
    DrawTextEx(headingFont, "API", {apiX + 13 * scale, rowY + 2 * scale},
               headingSize * theme::FONT_DETAIL, 1, theme::UI_TEXT);

    // Color row
    rowY += COLOR_ROW_SPACER * scale;
    DrawTextEx(headingFont, "Color:", {panel.x + SETTINGS_LABEL_X * scale, rowY},
               headingSize * theme::FONT_LABEL, 1, theme::UI_TEXT);

    const auto& types = highlighter.GetTypes();
    int activeId = highlighter.GetActiveTypeId();
    for (size_t i = 0; i < types.size(); ++i) {
        float swatchX = panel.x + COLOR_SWATCH_START * scale + i * (SWATCH_SIZE + SWATCH_GAP) * scale;
        Color c = {types[i].color.r, types[i].color.g, types[i].color.b, 255};
        DrawRectangle(swatchX, rowY, SWATCH_SIZE * scale, SWATCH_SIZE * scale, c);
        if (types[i].id == activeId) {
            DrawRectangleLines(swatchX - 1 * scale, rowY - 1 * scale,
                               SWATCH_SIZE * scale + 2 * scale, SWATCH_SIZE * scale + 2 * scale,
                               theme::PANEL_BORDER);
        } else {
            DrawRectangleLines(swatchX, rowY, SWATCH_SIZE * scale, SWATCH_SIZE * scale,
                               theme::BUTTON_BORDER);
        }
    }
}

SettingsPanel::Action SettingsPanel::HandleClick(Vector2 pos) {
    if (!active) return Action::None;

    Rectangle panel = GetPanelRect();
    if (!CheckCollisionPointRec(pos, panel)) {
        return Action::Dismiss;
    }

    if (HitTestClose(pos)) return Action::Dismiss;
    if (HitTestFontDecrease(pos)) return Action::FontDecrease;
    if (HitTestFontIncrease(pos)) return Action::FontIncrease;
    if (HitTestSourceOffline(pos)) return Action::SourceOffline;
    if (HitTestSourceOnline(pos)) return Action::SourceOnline;

    HandleColorClick(pos);
    return Action::None;
}

bool SettingsPanel::HitTestClose(Vector2 pos) const {
    return CheckCollisionPointRec(pos, GetCloseButtonRect(GetPanelRect()));
}

bool SettingsPanel::HitTestFontDecrease(Vector2 pos) const {
    Rectangle panel = GetPanelRect();
    float rowY = panel.y + SETTINGS_ROW1_Y * scale;
    Rectangle minusRect = {panel.x + FONT_BTN_X * scale, rowY, FONT_BTN_W * scale, FONT_BTN_H * scale};
    return CheckCollisionPointRec(pos, minusRect);
}

bool SettingsPanel::HitTestFontIncrease(Vector2 pos) const {
    Rectangle panel = GetPanelRect();
    float rowY = panel.y + SETTINGS_ROW1_Y * scale;
    Rectangle plusRect = {panel.x + FONT_PLUS_X * scale, rowY, FONT_BTN_W * scale, FONT_BTN_H * scale};
    return CheckCollisionPointRec(pos, plusRect);
}

bool SettingsPanel::HitTestSourceOffline(Vector2 pos) const {
    Rectangle panel = GetPanelRect();
    float rowY = panel.y + SETTINGS_ROW1_Y * scale + SETTINGS_ROW_GAP * scale;
    Rectangle usfmRect = {panel.x + SRC_USFM_X * scale, rowY, SRC_BTN_W * scale, SRC_BTN_H * scale};
    return CheckCollisionPointRec(pos, usfmRect);
}

bool SettingsPanel::HitTestSourceOnline(Vector2 pos) const {
    Rectangle panel = GetPanelRect();
    float rowY = panel.y + SETTINGS_ROW1_Y * scale + SETTINGS_ROW_GAP * scale;
    Rectangle onlineRect = {panel.x + SRC_ONLINE_X * scale, rowY, SRC_BTN_W * scale, SRC_BTN_H * scale};
    return CheckCollisionPointRec(pos, onlineRect);
}

void SettingsPanel::HandleColorClick(Vector2 pos) const {
    Rectangle panel = GetPanelRect();
    float rowY = panel.y + SETTINGS_ROW1_Y * scale + COLOR_ROW_SPACER * scale;
    const auto& types = highlighter.GetTypes();
    for (size_t i = 0; i < types.size(); ++i) {
        float swatchX = panel.x + COLOR_SWATCH_START * scale + i * (SWATCH_SIZE + SWATCH_GAP) * scale;
        Rectangle swatchRect = {swatchX, rowY, SWATCH_SIZE * scale, SWATCH_SIZE * scale};
        if (CheckCollisionPointRec(pos, swatchRect)) {
            highlighter.SetActiveTypeId(types[i].id);
            return;
        }
    }
}

} // namespace theword::renderer
