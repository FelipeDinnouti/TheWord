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
                     bool initialVersionOnline)
    : headingFont(headingFont), headingSize(headingSize), highlighter(highlighter),
      docManager(docManager), layoutEngine(layoutEngine), renderer(renderer),
      persistence(persistence), onlineProv(onlineProv), offlineProv(offlineProv),
      compositeProv(compositeProv),
      currentFontSize(initialFontSize), versionOnline(initialVersionOnline),
      contextMenuActive(false), contextMenuPos{0, 0},
      contextHighlightId(-1), contextHighlightTypeId(-1),
      goToDialogActive(false), goToSelection(0), goToError(false),
      aboutActive(false),
      settingsActive(false) {}

float UIManager::getContentTop() const {
    return TOP_BAR_HEIGHT;
}

float UIManager::getFontSize() const {
    return currentFontSize;
}

// ── Helpers ──────────────────────────────────────────────────────────────

void UIManager::drawBackdrop() {
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), theme::OVERLAY_BG);
}

void UIManager::applyFontSize(float newSize) {
    currentFontSize = newSize;
    layoutEngine.setFontSize(newSize);
    layoutEngine.invalidateCache();
    renderer.setFontSize(newSize);
    docManager.invalidateLayouts();
    persistence.setPreference("font_size", std::to_string((int)newSize));
}

Rectangle UIManager::getGoToDialogRect() const {
    float screenW = (float)GetScreenWidth();
    float screenH = (float)GetScreenHeight();
    return {(screenW - DIALOG_WIDTH) / 2.0f,
            (screenH - GO_TO_DIALOG_HEIGHT) / 2.0f,
            DIALOG_WIDTH, GO_TO_DIALOG_HEIGHT};
}

Rectangle UIManager::getSettingsPanelRect() const {
    float screenW = (float)GetScreenWidth();
    float screenH = (float)GetScreenHeight();
    return {(screenW - SETTINGS_WIDTH) / 2.0f,
            (screenH - SETTINGS_HEIGHT) / 2.0f,
            SETTINGS_WIDTH, SETTINGS_HEIGHT};
}

Rectangle UIManager::getCloseButtonRect(Rectangle panelRect) const {
    return {panelRect.x + panelRect.width - CLOSE_SIZE - CLOSE_MARGIN,
            panelRect.y + CLOSE_MARGIN,
            CLOSE_SIZE, CLOSE_SIZE};
}

// ── Top Bar ────────────────────────────────────────────────────────────

void UIManager::drawTopBar(const std::string& chapterTitle) {
    if (!chapterTitle.empty()) {
        DrawTextEx(headingFont, chapterTitle.c_str(), {20, 20}, headingSize, 1, theme::UI_TEXT);
    }
}

// ── Context Menu ───────────────────────────────────────────────────────

void UIManager::showContextMenu(Vector2 position, int highlightId, int typeId) {
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

void UIManager::hideContextMenu() {
    contextMenuActive = false;
}

bool UIManager::isContextMenuActive() const {
    return contextMenuActive;
}

bool UIManager::handleContextMenuClick(Vector2 pos) {
    if (!contextMenuActive) return false;

    Rectangle menuRect = {contextMenuPos.x, contextMenuPos.y, MENU_WIDTH, MENU_HEIGHT};
    if (!CheckCollisionPointRec(pos, menuRect)) {
        hideContextMenu();
        return false;
    }

    float x0 = contextMenuPos.x + MENU_PADDING;
    float y0 = contextMenuPos.y + MENU_PADDING;

    Rectangle deleteRect = {x0, y0, DELETE_WIDTH, MENU_HEIGHT - MENU_PADDING * 2};
    if (CheckCollisionPointRec(pos, deleteRect)) {
        highlighter.removeHighlight(contextHighlightId);
        hideContextMenu();
        return true;
    }

    float swatchStartX = x0 + DELETE_WIDTH + LABEL_SWATCH_GAP;
    const auto& types = highlighter.getTypes();
    for (size_t i = 0; i < types.size(); ++i) {
        float swatchX = swatchStartX + i * (SWATCH_SIZE + SWATCH_GAP);
        float swatchY = y0 + (MENU_HEIGHT - MENU_PADDING * 2 - SWATCH_SIZE) / 2.0f;
        if (CheckCollisionPointRec(pos, {swatchX, swatchY, SWATCH_SIZE, SWATCH_SIZE})) {
            highlighter.recolorHighlight(contextHighlightId, types[i].id);
            hideContextMenu();
            return true;
        }
    }

    hideContextMenu();
    return true;
}

void UIManager::drawContextMenu() {
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
    const auto& types = highlighter.getTypes();
    for (size_t i = 0; i < types.size(); ++i) {
        float swatchX = swatchStartX + i * (SWATCH_SIZE + SWATCH_GAP);
        Color c = {types[i].color.r, types[i].color.g, types[i].color.b, 255};
        DrawRectangle(swatchX, swatchY, SWATCH_SIZE, SWATCH_SIZE, c);
        DrawRectangleLines(swatchX, swatchY, SWATCH_SIZE, SWATCH_SIZE, theme::BUTTON_BORDER);
    }
}

// ── Go-To Dialog ───────────────────────────────────────────────────────

void UIManager::toggleGoToDialog() {
    goToDialogActive = !goToDialogActive;
    if (goToDialogActive) {
        goToInput.clear();
        goToSelection = 0;
    }
}

void UIManager::dismissGoToDialog() {
    goToDialogActive = false;
    goToInput.clear();
    goToError = false;
}

bool UIManager::isGoToDialogActive() const {
    return goToDialogActive;
}

bool UIManager::startsWithIgnoreCase(const std::string& str, const std::string& prefix) {
    if (str.size() < prefix.size()) return false;
    for (size_t i = 0; i < prefix.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(str[i])) !=
            std::tolower(static_cast<unsigned char>(prefix[i])))
            return false;
    }
    return true;
}

std::vector<int> UIManager::getSuggestions() const {
    std::vector<int> results;
    if (goToInput.empty()) return results;
    for (int i = 0; i < (int)BOOKS.size(); ++i) {
        if (startsWithIgnoreCase(goToInput, BOOKS[i].code) ||
            startsWithIgnoreCase(goToInput, BOOKS[i].fullName)) {
            results.push_back(i);
            if (results.size() >= 5) break;
        }
    }
    return results;
}

std::string UIManager::parseGoToInput(const std::string& input) const {
    if (auto r = tryParseBookDotChapter(input); !r.empty()) return r;
    if (auto r = tryParseFullNameThenChapter(input); !r.empty()) return r;
    if (auto r = tryParseSpaceSeparated(input); !r.empty()) return r;
    return "";
}

std::string UIManager::tryParseBookDotChapter(const std::string& input) {
    for (const auto& book : BOOKS) {
        std::string code = book.code;
        if (input.size() > code.size() + 1 &&
            startsWithIgnoreCase(input, code) && input[code.size()] == '.') {
            int ch = std::atoi(input.c_str() + code.size() + 1);
            if (ch >= 1 && ch <= book.chapterCount)
                return code + "." + std::to_string(ch);
        }
    }
    return "";
}

std::string UIManager::tryParseFullNameThenChapter(const std::string& input) {
    for (const auto& book : BOOKS) {
        std::string name = book.fullName;
        if (input.size() > name.size() + 1 &&
            startsWithIgnoreCase(input, name) && input[name.size()] == ' ') {
            int ch = std::atoi(input.c_str() + name.size() + 1);
            if (ch >= 1 && ch <= book.chapterCount)
                return std::string(book.code) + "." + std::to_string(ch);
        }
    }
    return "";
}

std::string UIManager::tryParseSpaceSeparated(const std::string& input) {
    size_t space = input.rfind(' ');
    if (space != std::string::npos && space + 1 < input.size()) {
        int ch = std::atoi(input.c_str() + space + 1);
        if (ch > 0) {
            std::string bookPart = input.substr(0, space);
            for (const auto& candidate : BOOKS) {
                if (startsWithIgnoreCase(bookPart, candidate.code) ||
                    startsWithIgnoreCase(bookPart, candidate.fullName)) {
                    if (ch <= candidate.chapterCount)
                        return std::string(candidate.code) + "." + std::to_string(ch);
                }
            }
        }
    }
    return "";
}

void UIManager::handleGoToKeyboardInput() {
    if (!goToDialogActive) return;

    int ch = GetCharPressed();
    while (ch > 0) {
        if (ch >= 32 && ch <= 126) {
            goToInput.push_back(static_cast<char>(ch));
            goToError = false;
        }
        ch = GetCharPressed();
    }

    if (IsKeyPressed(KEY_BACKSPACE) && !goToInput.empty()) {
        goToInput.pop_back();
        goToError = false;
    }

    if (IsKeyPressed(KEY_DOWN)) {
        auto suggestions = getSuggestions();
        goToSelection = std::min(goToSelection + 1, std::max(0, (int)suggestions.size() - 1));
    }
    if (IsKeyPressed(KEY_UP)) {
        goToSelection = std::max(goToSelection - 1, 0);
    }

    if (IsKeyPressed(KEY_TAB)) {
        auto suggestions = getSuggestions();
        if (!suggestions.empty() && goToSelection < (int)suggestions.size()) {
            goToInput = BOOKS[suggestions[goToSelection]].code;
        }
    }

    if (IsKeyPressed(KEY_ENTER) && !goToInput.empty()) {
        std::string ref = parseGoToInput(goToInput);
        if (!ref.empty()) {
            docManager.loadInitialChapter(ref);
            goToDialogActive = false;
            goToInput.clear();
            goToError = false;
        } else {
            goToError = true;
        }
    }
}

void UIManager::drawCloseButton(Rectangle panelRect) {
    Rectangle closeBtn = getCloseButtonRect(panelRect);
    DrawRectangleRec(closeBtn, theme::BUTTON_BG);
    DrawRectangleLinesEx(closeBtn, 1, theme::BUTTON_BORDER);
    Vector2 closeLabelSize = MeasureTextEx(headingFont, "X", headingSize * theme::FONT_DETAIL, 1);
    float closeTextX = closeBtn.x + (closeBtn.width - closeLabelSize.x) / 2.0f;
    DrawTextEx(headingFont, "X", {closeTextX, closeBtn.y + 1}, headingSize * theme::FONT_DETAIL, 1, theme::UI_TEXT);
}

void UIManager::drawGoToSuggestions(Rectangle dlg, const std::vector<int>& suggestions) {
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

void UIManager::drawGoToDialog() {
    if (!goToDialogActive) return;

    drawBackdrop();
    Rectangle dlgRect = getGoToDialogRect();

    DrawRectangleRec(dlgRect, theme::PANEL_BG);
    DrawRectangleLinesEx(dlgRect, 1, theme::PANEL_BORDER);

    DrawTextEx(headingFont, "Go to:", {dlgRect.x + SETTINGS_LABEL_X, dlgRect.y + SETTINGS_LABEL_X}, headingSize, 1, theme::UI_TITLE);
    drawCloseButton(dlgRect);

    Rectangle inputBox = {dlgRect.x + SETTINGS_LABEL_X, dlgRect.y + SETTINGS_ROW1_Y, DIALOG_WIDTH - 20, INPUT_BOX_H};
    DrawRectangleRec(inputBox, theme::INPUT_BG);
    DrawRectangleLinesEx(inputBox, 1, goToError ? theme::INPUT_BORDER_ERROR : theme::INPUT_BORDER);

    std::string display = goToInput;
    if (fmod(GetTime() * 2.0, 1.0) < 0.5) display += "|";
    DrawTextEx(headingFont, display.c_str(), {inputBox.x + INPUT_BOX_INSET, inputBox.y + INPUT_BOX_INSET}, headingSize, 1, theme::UI_INPUT_TEXT);

    drawGoToSuggestions(dlgRect, getSuggestions());
}

void UIManager::handleGoToClick(Vector2 pos) {
    if (!goToDialogActive) return;

    Rectangle dlgRect = getGoToDialogRect();
    if (!CheckCollisionPointRec(pos, dlgRect)) {
        dismissGoToDialog();
        return;
    }

    Rectangle closeBtn = getCloseButtonRect(dlgRect);
    if (CheckCollisionPointRec(pos, closeBtn)) {
        dismissGoToDialog();
    }
}

// ── About Overlay ──────────────────────────────────────────────────────

void UIManager::toggleAbout() {
    aboutActive = !aboutActive;
}

void UIManager::dismissAbout() {
    aboutActive = false;
}

bool UIManager::isAboutActive() const {
    return aboutActive;
}

Rectangle UIManager::getAboutRect() const {
    float screenW = (float)GetScreenWidth();
    float screenH = (float)GetScreenHeight();
    return {(screenW - ABOUT_WIDTH) / 2.0f,
            (screenH - ABOUT_HEIGHT) / 2.0f,
            ABOUT_WIDTH, ABOUT_HEIGHT};
}

void UIManager::drawAbout() {
    if (!aboutActive) return;

    drawBackdrop();
    Rectangle panel = getAboutRect();
    DrawRectangleRec(panel, theme::PANEL_BG);
    DrawRectangleLinesEx(panel, 1, theme::PANEL_BORDER);
    DrawTextEx(headingFont, "About", {panel.x + SETTINGS_LABEL_X, panel.y + SETTINGS_LABEL_X}, headingSize, 1, theme::UI_TITLE);
    drawCloseButton(panel);

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

void UIManager::handleAboutClick(Vector2 pos) {
    if (!aboutActive) return;

    Rectangle panel = getAboutRect();
    if (!CheckCollisionPointRec(pos, panel)) {
        dismissAbout();
        return;
    }
    Rectangle closeBtn = getCloseButtonRect(panel);
    if (CheckCollisionPointRec(pos, closeBtn)) {
        dismissAbout();
    }
}

// ── Settings Panel ─────────────────────────────────────────────────────

void UIManager::toggleSettings() {
    settingsActive = !settingsActive;
}

void UIManager::dismissSettings() {
    settingsActive = false;
}

bool UIManager::isSettingsActive() const {
    return settingsActive;
}

void UIManager::handleSettingsClick(Vector2 pos) {
    if (!settingsActive) return;

    Rectangle panelRect = getSettingsPanelRect();
    if (!CheckCollisionPointRec(pos, panelRect)) {
        settingsActive = false;
        return;
    }

    if (handleSettingsClickOnTitleClose(pos, panelRect)) return;
    if (handleSettingsClickOnFontRow(pos, panelRect)) return;
    if (compositeProv && handleSettingsClickOnSourceRow(pos, panelRect)) return;
    handleSettingsClickOnColorRow(pos, panelRect);
}

bool UIManager::handleSettingsClickOnTitleClose(Vector2 pos, Rectangle panel) {
    Rectangle closeBtn = getCloseButtonRect(panel);
    if (CheckCollisionPointRec(pos, closeBtn)) {
        settingsActive = false;
        return true;
    }
    return false;
}

bool UIManager::handleSettingsClickOnFontRow(Vector2 pos, Rectangle panel) {
    float rowY = panel.y + SETTINGS_ROW1_Y;
    Rectangle minusRect = {panel.x + FONT_BTN_X, rowY, FONT_BTN_W, FONT_BTN_H};
    if (CheckCollisionPointRec(pos, minusRect)) {
        applyFontSize(std::max(config::FONT_SIZE_MIN, currentFontSize - config::FONT_SIZE_STEP));
        return true;
    }
    Rectangle plusRect = {panel.x + FONT_PLUS_X, rowY, FONT_BTN_W, FONT_BTN_H};
    if (CheckCollisionPointRec(pos, plusRect)) {
        applyFontSize(std::min(config::FONT_SIZE_MAX, currentFontSize + config::FONT_SIZE_STEP));
        return true;
    }
    return false;
}

bool UIManager::handleSettingsClickOnSourceRow(Vector2 pos, Rectangle panel) {
    float rowY = panel.y + SETTINGS_ROW1_Y + SETTINGS_ROW_GAP;
    Rectangle usfmRect = {panel.x + SRC_USFM_X, rowY, SRC_BTN_W, SRC_BTN_H};
    Rectangle onlineRect = {panel.x + SRC_ONLINE_X, rowY, SRC_BTN_W, SRC_BTN_H};
    if (CheckCollisionPointRec(pos, usfmRect)) {
        compositeProv->setPrimary(offlineProv);
        versionOnline = false;
        highlighter.setProvider("USFMParser");
        docManager.loadInitialChapter(docManager.getCurrentChapterId());
        persistence.setPreference("active_version", "offline");
        return true;
    }
    if (CheckCollisionPointRec(pos, onlineRect)) {
        compositeProv->setPrimary(onlineProv);
        versionOnline = true;
        highlighter.setProvider("BibleClient");
        docManager.loadInitialChapter(docManager.getCurrentChapterId());
        persistence.setPreference("active_version", "online");
        return true;
    }
    return false;
}

void UIManager::handleSettingsClickOnColorRow(Vector2 pos, Rectangle panel) {
    float rowY = panel.y + SETTINGS_ROW1_Y + (compositeProv ? SETTINGS_ROW_GAP : 0) + COLOR_ROW_SPACER;
    const auto& types = highlighter.getTypes();
    for (size_t i = 0; i < types.size(); ++i) {
        float swatchX = panel.x + COLOR_SWATCH_START + i * (SWATCH_SIZE + SWATCH_GAP);
        Rectangle swatchRect = {swatchX, rowY, SWATCH_SIZE, SWATCH_SIZE};
        if (CheckCollisionPointRec(pos, swatchRect)) {
            highlighter.setActiveTypeId(types[i].id);
            persistence.setPreference("active_color", std::to_string(types[i].id));
            return;
        }
    }
}

void UIManager::drawSettingsPanel() {
    if (!settingsActive) return;

    drawBackdrop();
    Rectangle panelRect = getSettingsPanelRect();

    DrawRectangleRec(panelRect, theme::PANEL_BG);
    DrawRectangleLinesEx(panelRect, 1, theme::PANEL_BORDER);

    DrawTextEx(headingFont, "Settings", {panelRect.x + SETTINGS_LABEL_X, panelRect.y + SETTINGS_LABEL_X}, headingSize, 1, theme::UI_TITLE);
    drawCloseButton(panelRect);

    drawSettingsFontRow(panelRect);
    if (compositeProv) drawSettingsSourceRow(panelRect);
    drawSettingsColorRow(panelRect);
}

void UIManager::drawSettingsFontRow(Rectangle panel) {
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

void UIManager::drawSettingsSourceRow(Rectangle panel) {
    float rowY = panel.y + SETTINGS_ROW1_Y + SETTINGS_ROW_GAP;
    DrawTextEx(headingFont, "Source:", {panel.x + SETTINGS_LABEL_X, rowY}, headingSize * theme::FONT_LABEL, 1, theme::UI_TEXT);
    DrawRectangle(panel.x + SRC_USFM_X, rowY, SRC_BTN_W, SRC_BTN_H, versionOnline ? theme::SWITCH_OFF : theme::SWITCH_ON);
    DrawRectangleLines(panel.x + SRC_USFM_X, rowY, SRC_BTN_W, SRC_BTN_H, theme::BUTTON_BORDER);
    DrawTextEx(headingFont, "USFM", {panel.x + SRC_USFM_X + 10, rowY + 2}, headingSize * theme::FONT_DETAIL, 1, theme::UI_TEXT);
    DrawRectangle(panel.x + SRC_ONLINE_X, rowY, SRC_BTN_W, SRC_BTN_H, versionOnline ? theme::SWITCH_ON : theme::SWITCH_OFF);
    DrawRectangleLines(panel.x + SRC_ONLINE_X, rowY, SRC_BTN_W, SRC_BTN_H, theme::BUTTON_BORDER);
    DrawTextEx(headingFont, "API", {panel.x + SRC_ONLINE_X + 13, rowY + 2}, headingSize * theme::FONT_DETAIL, 1, theme::UI_TEXT);
}

void UIManager::drawSettingsColorRow(Rectangle panel) {
    float rowY = panel.y + SETTINGS_ROW1_Y + (compositeProv ? SETTINGS_ROW_GAP : 0) + COLOR_ROW_SPACER;
    DrawTextEx(headingFont, "Color:", {panel.x + SETTINGS_LABEL_X, rowY}, headingSize * theme::FONT_LABEL, 1, theme::UI_TEXT);
    const auto& types = highlighter.getTypes();
    int activeId = highlighter.getActiveTypeId();
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

// ── Dialog Dispatch ────────────────────────────────────────────────────

void UIManager::dismissActiveDialog() {
    if (aboutActive) dismissAbout();
    else if (goToDialogActive) dismissGoToDialog();
    else if (settingsActive) dismissSettings();
    else if (contextMenuActive) hideContextMenu();
}
