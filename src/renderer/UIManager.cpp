#include "UIManager.h"
#include "core/Config.h"
#include "highlight/Highlighter.h"
#include "document/DocumentManager.h"
#include "text/LayoutEngine.h"
#include "renderer/Renderer.h"
#include "persistence/PersistenceManager.h"
#include "data/CompositeProvider.h"
#include "core/BibleBooks.h"
#include <cctype>
#include <cstdlib>
#include <algorithm>
#include <cmath>

UIManager::UIManager(const Font& headingFont, float headingSize, Highlighter& highlighter,
                     DocumentManager& docManager, LayoutEngine& layoutEngine,
                     Renderer& renderer, PersistenceManager& persistence,
                     ChapterProvider& onlineProv, ChapterProvider& offlineProv,
                     CompositeProvider* compositeProv, float initialFontSize,
                     bool initialVersionOnline, float scaleFactor)
    : headingFont(headingFont), headingSize(headingSize), highlighter(highlighter),
      docManager(docManager), layoutEngine(layoutEngine), renderer(renderer),
      persistence(persistence), onlineProv(onlineProv), offlineProv(offlineProv),
      compositeProv(compositeProv),
      currentFontSize(initialFontSize), scale(scaleFactor), versionOnline(initialVersionOnline),
      contextMenuActive(false), contextMenuPos{0, 0},
      contextHighlightId(-1), contextHighlightTypeId(-1),
      goToDialogActive(false), goToSelection(0), goToError(false),
      aboutActive(false),
      settingsActive(false) {}

float UIManager::GetContentTop() const {
    return TOP_BAR_HEIGHT;
}

float UIManager::GetFontSize() const {
    return currentFontSize;
}

void UIManager::DrawBackdrop() {
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), theme::OVERLAY_BG);
}

void UIManager::ApplyFontSize(float newSize) {
    currentFontSize = newSize;
    float scaledSize = newSize * scale;
    layoutEngine.SetFontSize(scaledSize);
    layoutEngine.InvalidateCache();
    renderer.SetFontSize(scaledSize);
    docManager.InvalidateLayouts();
    persistence.SetPreference("font_size", std::to_string((int)newSize));
}

void UIManager::ChangeFontSize(float delta) {
    float newSize = currentFontSize + delta;
    newSize = std::max(config::FONT_SIZE_MIN, std::min(config::FONT_SIZE_MAX, newSize));
    if (newSize != currentFontSize) {
        ApplyFontSize(newSize);
    }
}

Rectangle UIManager::GetGoToDialogRect() const {
    float screenW = (float)GetScreenWidth();
    float screenH = (float)GetScreenHeight();
    return {(screenW - DIALOG_WIDTH) / 2.0f,
            (screenH - GO_TO_DIALOG_HEIGHT) / 2.0f,
            DIALOG_WIDTH, GO_TO_DIALOG_HEIGHT};
}

Rectangle UIManager::GetSettingsPanelRect() const {
    float screenW = (float)GetScreenWidth();
    float screenH = (float)GetScreenHeight();
    return {(screenW - SETTINGS_WIDTH) / 2.0f,
            (screenH - SETTINGS_HEIGHT) / 2.0f,
            SETTINGS_WIDTH, SETTINGS_HEIGHT};
}

Rectangle UIManager::GetCloseButtonRect(Rectangle panelRect) const {
    return {panelRect.x + panelRect.width - CLOSE_SIZE - CLOSE_MARGIN,
            panelRect.y + CLOSE_MARGIN,
            CLOSE_SIZE, CLOSE_SIZE};
}

void UIManager::DrawTopBar(const std::string& chapterTitle) {
    if (!chapterTitle.empty()) {
        DrawTextEx(headingFont, chapterTitle.c_str(), {20, 20}, headingSize, 1, theme::UI_TEXT);
    }
}

void UIManager::ShowContextMenu(Vector2 position, int highlightId, int typeId) {
    contextMenuActive = true;
    contextMenuPos = position;
    contextHighlightId = highlightId;
    contextHighlightTypeId = typeId;

    float screenW = (float)GetScreenWidth();
    float screenH = (float)GetScreenHeight();
    if (contextMenuPos.x + MENU_WIDTH > screenW)
        contextMenuPos.x = position.x - MENU_WIDTH - 4;
    if (contextMenuPos.y + MENU_HEIGHT > screenH)
        contextMenuPos.y = screenH - MENU_HEIGHT - 4;
    if (contextMenuPos.x < 0) contextMenuPos.x = 4;
    if (contextMenuPos.y < TOP_BAR_HEIGHT) contextMenuPos.y = TOP_BAR_HEIGHT;
}

void UIManager::HideContextMenu() {
    contextMenuActive = false;
}

bool UIManager::IsContextMenuActive() const {
    return contextMenuActive;
}

bool UIManager::HandleContextMenuClick(Vector2 pos) {
    if (!contextMenuActive) return false;

    Rectangle menuRect = {contextMenuPos.x, contextMenuPos.y, MENU_WIDTH, MENU_HEIGHT};
    if (!CheckCollisionPointRec(pos, menuRect)) {
        HideContextMenu();
        return false;
    }

    float x0 = contextMenuPos.x + MENU_PADDING;
    float y0 = contextMenuPos.y + MENU_PADDING;

    Rectangle deleteRect = {x0, y0, DELETE_WIDTH, MENU_HEIGHT - MENU_PADDING * 2};
    if (CheckCollisionPointRec(pos, deleteRect)) {
        highlighter.RemoveHighlight(contextHighlightId);
        HideContextMenu();
        return true;
    }

    float swatchStartX = x0 + DELETE_WIDTH + LABEL_SWATCH_GAP;
    const auto& types = highlighter.GetTypes();
    for (size_t i = 0; i < types.size(); ++i) {
        float swatchX = swatchStartX + i * (SWATCH_SIZE + SWATCH_GAP);
        float swatchY = y0 + (MENU_HEIGHT - MENU_PADDING * 2 - SWATCH_SIZE) / 2.0f;
        if (CheckCollisionPointRec(pos, {swatchX, swatchY, SWATCH_SIZE, SWATCH_SIZE})) {
            highlighter.RecolorHighlight(contextHighlightId, types[i].id);
            HideContextMenu();
            return true;
        }
    }

    HideContextMenu();
    return true;
}

void UIManager::DrawContextMenu() {
    if (!contextMenuActive) return;

    DrawRectangle(contextMenuPos.x, contextMenuPos.y, MENU_WIDTH, MENU_HEIGHT, theme::PANEL_BG);
    DrawRectangleLines(contextMenuPos.x, contextMenuPos.y, MENU_WIDTH, MENU_HEIGHT, theme::PANEL_BORDER);

    float x0 = contextMenuPos.x + MENU_PADDING;
    float y0 = contextMenuPos.y + MENU_PADDING;
    float contentH = MENU_HEIGHT - MENU_PADDING * 2;

    DrawTextEx(headingFont, "Del", {x0, y0 + (contentH - headingSize) / 2.0f},
               headingSize, 1, theme::UI_DELETE);

    float swatchStartX = x0 + DELETE_WIDTH + LABEL_SWATCH_GAP;
    float swatchY = y0 + (contentH - SWATCH_SIZE) / 2.0f;
    const auto& types = highlighter.GetTypes();
    for (size_t i = 0; i < types.size(); ++i) {
        float swatchX = swatchStartX + i * (SWATCH_SIZE + SWATCH_GAP);
        Color c = {types[i].color.r, types[i].color.g, types[i].color.b, 255};
        DrawRectangle(swatchX, swatchY, SWATCH_SIZE, SWATCH_SIZE, c);
        DrawRectangleLines(swatchX, swatchY, SWATCH_SIZE, SWATCH_SIZE, theme::BUTTON_BORDER);
    }
}

void UIManager::ToggleGoToDialog() {
    goToDialogActive = !goToDialogActive;
    if (goToDialogActive) {
        goToInput.clear();
        goToSelection = 0;
    }
}

void UIManager::DismissGoToDialog() {
    goToDialogActive = false;
    goToInput.clear();
    goToError = false;
}

bool UIManager::IsGoToDialogActive() const {
    return goToDialogActive;
}

bool UIManager::StartsWithIgnoreCase(const std::string& str, const std::string& prefix) {
    if (str.size() < prefix.size()) return false;
    for (size_t i = 0; i < prefix.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(str[i])) !=
            std::tolower(static_cast<unsigned char>(prefix[i])))
            return false;
    }
    return true;
}

std::vector<int> UIManager::GetSuggestions() const {
    std::vector<int> results;
    if (goToInput.empty()) return results;
    for (int i = 0; i < (int)BOOKS.size(); ++i) {
        if (StartsWithIgnoreCase(goToInput, BOOKS[i].code) ||
            StartsWithIgnoreCase(goToInput, BOOKS[i].fullName)) {
            results.push_back(i);
            if (results.size() >= 5) break;
        }
    }
    return results;
}

std::string UIManager::ParseGoToInput(const std::string& input) const {
    if (auto r = TryParseBookDotChapter(input); !r.empty()) return r;
    if (auto r = TryParseFullNameThenChapter(input); !r.empty()) return r;
    if (auto r = TryParseSpaceSeparated(input); !r.empty()) return r;
    return "";
}

std::string UIManager::TryParseBookDotChapter(const std::string& input) {
    for (const auto& book : BOOKS) {
        std::string code = book.code;
        if (input.size() > code.size() + 1 &&
            StartsWithIgnoreCase(input, code) && input[code.size()] == '.') {
            int ch = std::atoi(input.c_str() + code.size() + 1);
            if (ch >= 1 && ch <= book.chapterCount)
                return code + "." + std::to_string(ch);
        }
    }
    return "";
}

std::string UIManager::TryParseFullNameThenChapter(const std::string& input) {
    for (const auto& book : BOOKS) {
        std::string name = book.fullName;
        if (input.size() > name.size() + 1 &&
            StartsWithIgnoreCase(input, name) && input[name.size()] == ' ') {
            int ch = std::atoi(input.c_str() + name.size() + 1);
            if (ch >= 1 && ch <= book.chapterCount)
                return std::string(book.code) + "." + std::to_string(ch);
        }
    }
    return "";
}

std::string UIManager::TryParseSpaceSeparated(const std::string& input) {
    size_t space = input.rfind(' ');
    if (space != std::string::npos && space + 1 < input.size()) {
        int ch = std::atoi(input.c_str() + space + 1);
        if (ch > 0) {
            std::string bookPart = input.substr(0, space);
            for (const auto& candidate : BOOKS) {
                if (StartsWithIgnoreCase(bookPart, candidate.code) ||
                    StartsWithIgnoreCase(bookPart, candidate.fullName)) {
                    if (ch <= candidate.chapterCount)
                        return std::string(candidate.code) + "." + std::to_string(ch);
                }
            }
        }
    }
    return "";
}

void UIManager::HandleGoToKeyboardInput() {
    if (!goToDialogActive) return;

    int ch = GetCharPressed();
    while (ch > 0) {
        if (ch >= 32 && ch <= 126) {
            goToInput.push_back(static_cast<char>(ch));
            goToError = false;
        }
        ch = GetCharPressed();
    }

    if (IsKeyPressed(key::BACKSPACE) && !goToInput.empty()) {
        goToInput.pop_back();
        goToError = false;
    }

    if (IsKeyPressed(key::DOWN)) {
        auto suggestions = GetSuggestions();
        goToSelection = std::min(goToSelection + 1, std::max(0, (int)suggestions.size() - 1));
    }
    if (IsKeyPressed(key::UP)) {
        goToSelection = std::max(goToSelection - 1, 0);
    }

    if (IsKeyPressed(key::TAB)) {
        auto suggestions = GetSuggestions();
        if (!suggestions.empty() && goToSelection < (int)suggestions.size()) {
            goToInput = BOOKS[suggestions[goToSelection]].code;
        }
    }

    if (IsKeyPressed(key::ENTER) && !goToInput.empty()) {
        std::string ref = ParseGoToInput(goToInput);
        if (!ref.empty()) {
            docManager.LoadInitialChapter(ref);
            goToDialogActive = false;
            goToInput.clear();
            goToError = false;
        } else {
            goToError = true;
        }
    }
}

void UIManager::DrawCloseButton(Rectangle panelRect) {
    Rectangle closeBtn = GetCloseButtonRect(panelRect);
    DrawRectangleRec(closeBtn, theme::BUTTON_BG);
    DrawRectangleLinesEx(closeBtn, 1, theme::BUTTON_BORDER);
    Vector2 closeLabelSize = MeasureTextEx(headingFont, "X", headingSize * theme::FONT_DETAIL, 1);
    float closeTextX = closeBtn.x + (closeBtn.width - closeLabelSize.x) / 2.0f;
    DrawTextEx(headingFont, "X", {closeTextX, closeBtn.y + 1}, headingSize * theme::FONT_DETAIL, 1, theme::UI_TEXT);
}

void UIManager::DrawGoToSuggestions(Rectangle dlg, const std::vector<int>& suggestions) {
    float sy = dlg.y + 80;
    for (size_t i = 0; i < suggestions.size(); ++i) {
        const auto& book = BOOKS[suggestions[i]];
        Color bg = ((int)i == goToSelection) ? theme::SELECTED_BG : theme::PANEL_BG;
        DrawRectangle(dlg.x + 10, sy, DIALOG_WIDTH - 20, SUGGESTION_ITEM_H, bg);
        std::string label = std::string(book.code) + " - " + book.fullName;
        DrawTextEx(headingFont, label.c_str(), {dlg.x + 14, sy + 2}, headingSize * theme::FONT_LABEL, 1, theme::UI_TEXT);
        sy += SUGGESTION_LINE_H;
    }
}

void UIManager::DrawGoToDialog() {
    if (!goToDialogActive) return;

    DrawBackdrop();
    Rectangle dlgRect = GetGoToDialogRect();

    DrawRectangleRec(dlgRect, theme::PANEL_BG);
    DrawRectangleLinesEx(dlgRect, 1, theme::PANEL_BORDER);

    DrawTextEx(headingFont, "Go to:", {dlgRect.x + SETTINGS_LABEL_X, dlgRect.y + SETTINGS_LABEL_X}, headingSize, 1, theme::UI_TITLE);
    DrawCloseButton(dlgRect);

    Rectangle inputBox = {dlgRect.x + SETTINGS_LABEL_X, dlgRect.y + SETTINGS_ROW1_Y, DIALOG_WIDTH - 20, INPUT_BOX_H};
    DrawRectangleRec(inputBox, theme::INPUT_BG);
    DrawRectangleLinesEx(inputBox, 1, goToError ? theme::INPUT_BORDER_ERROR : theme::INPUT_BORDER);

    std::string display = goToInput;
    if (fmod(GetTime() * 2.0, 1.0) < 0.5) display += "|";
    DrawTextEx(headingFont, display.c_str(), {inputBox.x + INPUT_BOX_INSET, inputBox.y + INPUT_BOX_INSET}, headingSize, 1, theme::UI_INPUT_TEXT);

    DrawGoToSuggestions(dlgRect, GetSuggestions());
}

void UIManager::HandleGoToClick(Vector2 pos) {
    if (!goToDialogActive) return;

    Rectangle dlgRect = GetGoToDialogRect();
    if (!CheckCollisionPointRec(pos, dlgRect)) {
        DismissGoToDialog();
        return;
    }

    Rectangle closeBtn = GetCloseButtonRect(dlgRect);
    if (CheckCollisionPointRec(pos, closeBtn)) {
        DismissGoToDialog();
    }
}

void UIManager::ToggleAbout() {
    aboutActive = !aboutActive;
}

void UIManager::DismissAbout() {
    aboutActive = false;
}

bool UIManager::IsAboutActive() const {
    return aboutActive;
}

Rectangle UIManager::GetAboutRect() const {
    float screenW = (float)GetScreenWidth();
    float screenH = (float)GetScreenHeight();
    return {(screenW - ABOUT_WIDTH) / 2.0f,
            (screenH - ABOUT_HEIGHT) / 2.0f,
            ABOUT_WIDTH, ABOUT_HEIGHT};
}

void UIManager::DrawAbout() {
    if (!aboutActive) return;

    DrawBackdrop();
    Rectangle panel = GetAboutRect();
    DrawRectangleRec(panel, theme::PANEL_BG);
    DrawRectangleLinesEx(panel, 1, theme::PANEL_BORDER);
    DrawTextEx(headingFont, "About", {panel.x + SETTINGS_LABEL_X, panel.y + SETTINGS_LABEL_X}, headingSize, 1, theme::UI_TITLE);
    DrawCloseButton(panel);

    float y = panel.y + ABOUT_FIRST_LINE_Y;
    DrawTextEx(headingFont, "TheWord Bible Reader", {panel.x + SETTINGS_LABEL_X, y}, headingSize * theme::FONT_DETAIL, 1, theme::UI_TEXT);
    y += ABOUT_TEXT_LINE_H;
    DrawTextEx(headingFont, "Built with Raylib", {panel.x + SETTINGS_LABEL_X, y}, headingSize * theme::FONT_DETAIL, 1, theme::UI_TEXT);
    y += ABOUT_TEXT_LINE_H;
    DrawTextEx(headingFont, "Data: Bilia Livre (CC BY 4.0)", {panel.x + SETTINGS_LABEL_X, y}, headingSize * theme::FONT_DETAIL, 1, theme::UI_TEXT);
    y += ABOUT_TEXT_LINE_H;
    DrawTextEx(headingFont, "API: YouVersion", {panel.x + SETTINGS_LABEL_X, y}, headingSize * theme::FONT_DETAIL, 1, theme::UI_TEXT);
    y += ABOUT_TEXT_LINE_H;
    DrawTextEx(headingFont, "Keyboard: G/S/A/Esc", {panel.x + SETTINGS_LABEL_X, y}, headingSize * theme::FONT_DETAIL, 1, theme::UI_TEXT);
}

void UIManager::HandleAboutClick(Vector2 pos) {
    if (!aboutActive) return;

    Rectangle panel = GetAboutRect();
    if (!CheckCollisionPointRec(pos, panel)) {
        DismissAbout();
        return;
    }
    Rectangle closeBtn = GetCloseButtonRect(panel);
    if (CheckCollisionPointRec(pos, closeBtn)) {
        DismissAbout();
    }
}

void UIManager::ToggleSettings() {
    settingsActive = !settingsActive;
}

void UIManager::DismissSettings() {
    settingsActive = false;
}

bool UIManager::IsSettingsActive() const {
    return settingsActive;
}

void UIManager::HandleSettingsClick(Vector2 pos) {
    if (!settingsActive) return;

    Rectangle panelRect = GetSettingsPanelRect();
    if (!CheckCollisionPointRec(pos, panelRect)) {
        settingsActive = false;
        return;
    }

    if (HandleSettingsClickOnTitleClose(pos, panelRect)) return;
    if (HandleSettingsClickOnFontRow(pos, panelRect)) return;
    if (compositeProv && HandleSettingsClickOnSourceRow(pos, panelRect)) return;
    HandleSettingsClickOnColorRow(pos, panelRect);
}

bool UIManager::HandleSettingsClickOnTitleClose(Vector2 pos, Rectangle panel) {
    Rectangle closeBtn = GetCloseButtonRect(panel);
    if (CheckCollisionPointRec(pos, closeBtn)) {
        settingsActive = false;
        return true;
    }
    return false;
}

bool UIManager::HandleSettingsClickOnFontRow(Vector2 pos, Rectangle panel) {
    float rowY = panel.y + SETTINGS_ROW1_Y;
    Rectangle minusRect = {panel.x + FONT_BTN_X, rowY, FONT_BTN_W, FONT_BTN_H};
    if (CheckCollisionPointRec(pos, minusRect)) {
        ApplyFontSize(std::max(config::FONT_SIZE_MIN, currentFontSize - config::FONT_SIZE_STEP));
        return true;
    }
    Rectangle plusRect = {panel.x + FONT_PLUS_X, rowY, FONT_BTN_W, FONT_BTN_H};
    if (CheckCollisionPointRec(pos, plusRect)) {
        ApplyFontSize(std::min(config::FONT_SIZE_MAX, currentFontSize + config::FONT_SIZE_STEP));
        return true;
    }
    return false;
}

bool UIManager::HandleSettingsClickOnSourceRow(Vector2 pos, Rectangle panel) {
    float rowY = panel.y + SETTINGS_ROW1_Y + SETTINGS_ROW_GAP;
    Rectangle usfmRect = {panel.x + SRC_USFM_X, rowY, SRC_BTN_W, SRC_BTN_H};
    Rectangle onlineRect = {panel.x + SRC_ONLINE_X, rowY, SRC_BTN_W, SRC_BTN_H};
    if (CheckCollisionPointRec(pos, usfmRect)) {
        compositeProv->SetPrimary(offlineProv);
        versionOnline = false;
        highlighter.SetProvider("USFMParser");
        docManager.LoadInitialChapter(docManager.GetCurrentChapterId());
        persistence.SetPreference("active_version", "offline");
        return true;
    }
    if (CheckCollisionPointRec(pos, onlineRect)) {
        compositeProv->SetPrimary(onlineProv);
        versionOnline = true;
        highlighter.SetProvider("BibleClient");
        docManager.LoadInitialChapter(docManager.GetCurrentChapterId());
        persistence.SetPreference("active_version", "online");
        return true;
    }
    return false;
}

void UIManager::HandleSettingsClickOnColorRow(Vector2 pos, Rectangle panel) {
    float rowY = panel.y + SETTINGS_ROW1_Y + (compositeProv ? SETTINGS_ROW_GAP : 0) + COLOR_ROW_SPACER;
    const auto& types = highlighter.GetTypes();
    for (size_t i = 0; i < types.size(); ++i) {
        float swatchX = panel.x + COLOR_SWATCH_START + i * (SWATCH_SIZE + SWATCH_GAP);
        Rectangle swatchRect = {swatchX, rowY, SWATCH_SIZE, SWATCH_SIZE};
        if (CheckCollisionPointRec(pos, swatchRect)) {
            highlighter.SetActiveTypeId(types[i].id);
            persistence.SetPreference("active_color", std::to_string(types[i].id));
            return;
        }
    }
}

void UIManager::DrawSettingsPanel() {
    if (!settingsActive) return;

    DrawBackdrop();
    Rectangle panelRect = GetSettingsPanelRect();

    DrawRectangleRec(panelRect, theme::PANEL_BG);
    DrawRectangleLinesEx(panelRect, 1, theme::PANEL_BORDER);

    DrawTextEx(headingFont, "Settings", {panelRect.x + SETTINGS_LABEL_X, panelRect.y + SETTINGS_LABEL_X}, headingSize, 1, theme::UI_TITLE);
    DrawCloseButton(panelRect);

    DrawSettingsFontRow(panelRect);
    if (compositeProv) DrawSettingsSourceRow(panelRect);
    DrawSettingsColorRow(panelRect);
}

void UIManager::DrawSettingsFontRow(Rectangle panel) {
    float rowY = panel.y + SETTINGS_ROW1_Y;
    DrawTextEx(headingFont, "Font:", {panel.x + SETTINGS_LABEL_X, rowY}, headingSize * theme::FONT_LABEL, 1, theme::UI_TEXT);
    DrawRectangle(panel.x + FONT_BTN_X, rowY, FONT_BTN_W, FONT_BTN_H, theme::BUTTON_BG);
    DrawRectangleLines(panel.x + FONT_BTN_X, rowY, FONT_BTN_W, FONT_BTN_H, currentFontSize <= config::FONT_SIZE_MIN ? theme::BUTTON_BORDER_DISABLED : theme::BUTTON_BORDER);
    DrawTextEx(headingFont, "-", {panel.x + FONT_BTN_X + 8, rowY + 1}, headingSize * theme::FONT_LABEL, 1, currentFontSize <= config::FONT_SIZE_MIN ? theme::UI_BUTTON_TEXT_DISABLED : theme::UI_BUTTON_TEXT);
    std::string sizeStr = std::to_string((int)currentFontSize);
    Vector2 sz = MeasureTextEx(headingFont, sizeStr.c_str(), headingSize * theme::FONT_LABEL, 1);
    DrawTextEx(headingFont, sizeStr.c_str(), {panel.x + FONT_SIZE_X - sz.x / 2, rowY + 2}, headingSize * theme::FONT_LABEL, 1, theme::UI_TEXT);
    DrawRectangle(panel.x + FONT_PLUS_X, rowY, FONT_BTN_W, FONT_BTN_H, theme::BUTTON_BG);
    DrawRectangleLines(panel.x + FONT_PLUS_X, rowY, FONT_BTN_W, FONT_BTN_H, currentFontSize >= config::FONT_SIZE_MAX ? theme::BUTTON_BORDER_DISABLED : theme::BUTTON_BORDER);
    DrawTextEx(headingFont, "+", {panel.x + FONT_PLUS_X + 8, rowY + 1}, headingSize * theme::FONT_LABEL, 1, currentFontSize >= config::FONT_SIZE_MAX ? theme::UI_BUTTON_TEXT_DISABLED : theme::UI_BUTTON_TEXT);
}

void UIManager::DrawSettingsSourceRow(Rectangle panel) {
    float rowY = panel.y + SETTINGS_ROW1_Y + SETTINGS_ROW_GAP;
    DrawTextEx(headingFont, "Source:", {panel.x + SETTINGS_LABEL_X, rowY}, headingSize * theme::FONT_LABEL, 1, theme::UI_TEXT);
    DrawRectangle(panel.x + SRC_USFM_X, rowY, SRC_BTN_W, SRC_BTN_H, versionOnline ? theme::SWITCH_OFF : theme::SWITCH_ON);
    DrawRectangleLines(panel.x + SRC_USFM_X, rowY, SRC_BTN_W, SRC_BTN_H, theme::BUTTON_BORDER);
    DrawTextEx(headingFont, "USFM", {panel.x + SRC_USFM_X + 10, rowY + 2}, headingSize * theme::FONT_DETAIL, 1, theme::UI_TEXT);
    DrawRectangle(panel.x + SRC_ONLINE_X, rowY, SRC_BTN_W, SRC_BTN_H, versionOnline ? theme::SWITCH_ON : theme::SWITCH_OFF);
    DrawRectangleLines(panel.x + SRC_ONLINE_X, rowY, SRC_BTN_W, SRC_BTN_H, theme::BUTTON_BORDER);
    DrawTextEx(headingFont, "API", {panel.x + SRC_ONLINE_X + 13, rowY + 2}, headingSize * theme::FONT_DETAIL, 1, theme::UI_TEXT);
}

void UIManager::DrawSettingsColorRow(Rectangle panel) {
    float rowY = panel.y + SETTINGS_ROW1_Y + (compositeProv ? SETTINGS_ROW_GAP : 0) + COLOR_ROW_SPACER;
    DrawTextEx(headingFont, "Color:", {panel.x + SETTINGS_LABEL_X, rowY}, headingSize * theme::FONT_LABEL, 1, theme::UI_TEXT);
    const auto& types = highlighter.GetTypes();
    int activeId = highlighter.GetActiveTypeId();
    for (size_t i = 0; i < types.size(); ++i) {
        float swatchX = panel.x + COLOR_SWATCH_START + i * (SWATCH_SIZE + SWATCH_GAP);
        Color c = {types[i].color.r, types[i].color.g, types[i].color.b, 255};
        DrawRectangle(swatchX, rowY, SWATCH_SIZE, SWATCH_SIZE, c);
        if (types[i].id == activeId) {
            DrawRectangleLines(swatchX - 1, rowY - 1, SWATCH_SIZE + 2, SWATCH_SIZE + 2, theme::PANEL_BORDER);
        } else {
            DrawRectangleLines(swatchX, rowY, SWATCH_SIZE, SWATCH_SIZE, theme::BUTTON_BORDER);
        }
    }
}

void UIManager::DismissActiveDialog() {
    if (aboutActive) DismissAbout();
    else if (goToDialogActive) DismissGoToDialog();
    else if (settingsActive) DismissSettings();
    else if (contextMenuActive) HideContextMenu();
}
