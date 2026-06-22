#include "UIManager.h"
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
      goToDialogActive(false), goToSelection(0),
      settingsActive(false) {}

float UIManager::getContentTop() const {
    return TOP_BAR_HEIGHT;
}

float UIManager::getFontSize() const {
    return currentFontSize;
}

// ── Helpers ──────────────────────────────────────────────────────────────

void UIManager::drawBackdrop() {
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(),
                  {0, 0, 0, BACKDROP_ALPHA});
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
        DrawTextEx(headingFont, chapterTitle.c_str(), {20, 20}, headingSize, 1, DARKGRAY);
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

    DrawRectangle(contextMenuPos.x, contextMenuPos.y, MENU_WIDTH, MENU_HEIGHT, WHITE);
    DrawRectangleLines(contextMenuPos.x, contextMenuPos.y, MENU_WIDTH, MENU_HEIGHT, DARKGRAY);

    float x0 = contextMenuPos.x + MENU_PADDING;
    float y0 = contextMenuPos.y + MENU_PADDING;
    float contentH = MENU_HEIGHT - MENU_PADDING * 2;

    DrawTextEx(headingFont, "Del", {x0, y0 + (contentH - headingSize) / 2.0f},
               headingSize, 1, RED);

    float swatchStartX = x0 + DELETE_WIDTH + LABEL_SWATCH_GAP;
    float swatchY = y0 + (contentH - SWATCH_SIZE) / 2.0f;
    const auto& types = highlighter.getTypes();
    for (size_t i = 0; i < types.size(); ++i) {
        float swatchX = swatchStartX + i * (SWATCH_SIZE + SWATCH_GAP);
        Color c = {types[i].color.r, types[i].color.g, types[i].color.b, 255};
        DrawRectangle(swatchX, swatchY, SWATCH_SIZE, SWATCH_SIZE, c);
        DrawRectangleLines(swatchX, swatchY, SWATCH_SIZE, SWATCH_SIZE, GRAY);
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
    for (const auto& book : BOOKS) {
        std::string code = book.code;

        if (input.size() > code.size() + 1 &&
            startsWithIgnoreCase(input, code) && input[code.size()] == '.') {
            int ch = std::atoi(input.c_str() + code.size() + 1);
            if (ch >= 1 && ch <= book.chapterCount)
                return code + "." + std::to_string(ch);
        }

        std::string name = book.fullName;
        if (input.size() > name.size() + 1 &&
            startsWithIgnoreCase(input, name) && input[name.size()] == ' ') {
            int ch = std::atoi(input.c_str() + name.size() + 1);
            if (ch >= 1 && ch <= book.chapterCount)
                return code + "." + std::to_string(ch);
        }
    }

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
        }
        ch = GetCharPressed();
    }

    if (IsKeyPressed(KEY_BACKSPACE) && !goToInput.empty()) {
        goToInput.pop_back();
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
        }
    }
}

void UIManager::drawGoToDialog() {
    if (!goToDialogActive) return;

    drawBackdrop();
    Rectangle dlgRect = getGoToDialogRect();

    DrawRectangleRec(dlgRect, WHITE);
    DrawRectangleLinesEx(dlgRect, 1, DARKGRAY);

    DrawTextEx(headingFont, "Go to:", {dlgRect.x + 10, dlgRect.y + 10}, headingSize, 1, BLACK);

    Rectangle closeBtn = getCloseButtonRect(dlgRect);
    DrawRectangleRec(closeBtn, LIGHTGRAY);
    DrawRectangleLinesEx(closeBtn, 1, GRAY);

    Vector2 closeLabelSize = MeasureTextEx(headingFont, "X", headingSize * 0.7f, 1);
    float closeTextX = closeBtn.x + (closeBtn.width - closeLabelSize.x) / 2.0f;
    DrawTextEx(headingFont, "X", {closeTextX, closeBtn.y + 1}, headingSize * 0.7f, 1, DARKGRAY);

    Rectangle inputBox = {dlgRect.x + 10, dlgRect.y + 40, DIALOG_WIDTH - 20, 30};
    DrawRectangleRec(inputBox, LIGHTGRAY);
    DrawRectangleLinesEx(inputBox, 1, GRAY);

    std::string display = goToInput;
    if (fmod(GetTime() * 2.0, 1.0) < 0.5) display += "|";
    DrawTextEx(headingFont, display.c_str(), {inputBox.x + 4, inputBox.y + 4}, headingSize, 1, BLACK);

    auto suggestions = getSuggestions();
    float sy = dlgRect.y + 80;
    for (size_t i = 0; i < suggestions.size(); ++i) {
        const auto& book = BOOKS[suggestions[i]];
        Color bg = ((int)i == goToSelection) ? SKYBLUE : WHITE;
        DrawRectangle(dlgRect.x + 10, sy, DIALOG_WIDTH - 20, 22, bg);
        std::string label = std::string(book.code) + " - " + book.fullName;
        DrawTextEx(headingFont, label.c_str(), {dlgRect.x + 14, sy + 2}, headingSize * 0.8f, 1, DARKGRAY);
        sy += 24;
    }
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

    Rectangle closeBtn = getCloseButtonRect(panelRect);
    if (CheckCollisionPointRec(pos, closeBtn)) {
        settingsActive = false;
        return;
    }

    float row1y = panelRect.y + 40;
    float row2y = row1y + 30;

    Rectangle minusRect = {panelRect.x + 120, row1y, 28, 22};
    if (CheckCollisionPointRec(pos, minusRect)) {
        applyFontSize(std::max(12.0f, currentFontSize - 2.0f));
        return;
    }

    Rectangle plusRect = {panelRect.x + 180, row1y, 28, 22};
    if (CheckCollisionPointRec(pos, plusRect)) {
        applyFontSize(std::min(36.0f, currentFontSize + 2.0f));
        return;
    }

    if (compositeProv) {
        Rectangle usfmRect = {panelRect.x + 100, row2y, 60, 22};
        Rectangle onlineRect = {panelRect.x + 168, row2y, 60, 22};
        if (CheckCollisionPointRec(pos, usfmRect)) {
            compositeProv->setPrimary(offlineProv);
            versionOnline = false;
            highlighter.setProvider("USFMParser");
            docManager.loadInitialChapter(docManager.getCurrentChapterId());
            persistence.setPreference("active_version", "offline");
            return;
        }
        if (CheckCollisionPointRec(pos, onlineRect)) {
            compositeProv->setPrimary(onlineProv);
            versionOnline = true;
            highlighter.setProvider("BibleClient");
            docManager.loadInitialChapter(docManager.getCurrentChapterId());
            persistence.setPreference("active_version", "online");
            return;
        }
    }

    float row3y = row2y + 40;
    const auto& types = highlighter.getTypes();
    for (size_t i = 0; i < types.size(); ++i) {
        float swatchX = panelRect.x + 30 + i * (SWATCH_SIZE + SWATCH_GAP);
        Rectangle swatchRect = {swatchX, row3y, SWATCH_SIZE, SWATCH_SIZE};
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

    DrawRectangleRec(panelRect, WHITE);
    DrawRectangleLinesEx(panelRect, 1, DARKGRAY);

    DrawTextEx(headingFont, "Settings", {panelRect.x + 10, panelRect.y + 10}, headingSize, 1, BLACK);

    Rectangle closeBtn = getCloseButtonRect(panelRect);
    DrawRectangleRec(closeBtn, LIGHTGRAY);
    DrawRectangleLinesEx(closeBtn, 1, GRAY);
    Vector2 closeLabelSize = MeasureTextEx(headingFont, "X", headingSize * 0.7f, 1);
    float closeTextX = closeBtn.x + (closeBtn.width - closeLabelSize.x) / 2.0f;
    DrawTextEx(headingFont, "X", {closeTextX, closeBtn.y + 1}, headingSize * 0.7f, 1, DARKGRAY);

    float row1y = panelRect.y + 40;
    DrawTextEx(headingFont, "Font:", {panelRect.x + 10, row1y}, headingSize * 0.8f, 1, DARKGRAY);
    DrawRectangle(panelRect.x + 120, row1y, 28, 22, LIGHTGRAY);
    DrawRectangleLines(panelRect.x + 120, row1y, 28, 22, currentFontSize <= 12.0f ? LIGHTGRAY : GRAY);
    DrawTextEx(headingFont, "-", {panelRect.x + 128, row1y + 1}, headingSize * 0.8f, 1, currentFontSize <= 12.0f ? GRAY : BLACK);
    std::string sizeStr = std::to_string((int)currentFontSize);
    Vector2 sz = MeasureTextEx(headingFont, sizeStr.c_str(), headingSize * 0.8f, 1);
    DrawTextEx(headingFont, sizeStr.c_str(), {panelRect.x + 154 - sz.x / 2, row1y + 2}, headingSize * 0.8f, 1, DARKGRAY);
    DrawRectangle(panelRect.x + 180, row1y, 28, 22, LIGHTGRAY);
    DrawRectangleLines(panelRect.x + 180, row1y, 28, 22, currentFontSize >= 36.0f ? LIGHTGRAY : GRAY);
    DrawTextEx(headingFont, "+", {panelRect.x + 188, row1y + 1}, headingSize * 0.8f, 1, currentFontSize >= 36.0f ? GRAY : BLACK);

    if (compositeProv) {
        float row2y = row1y + 30;
        DrawTextEx(headingFont, "Source:", {panelRect.x + 10, row2y}, headingSize * 0.8f, 1, DARKGRAY);
        DrawRectangle(panelRect.x + 100, row2y, 60, 22, versionOnline ? LIGHTGRAY : SKYBLUE);
        DrawRectangleLines(panelRect.x + 100, row2y, 60, 22, GRAY);
        DrawTextEx(headingFont, "USFM", {panelRect.x + 110, row2y + 2}, headingSize * 0.7f, 1, DARKGRAY);
        DrawRectangle(panelRect.x + 168, row2y, 60, 22, versionOnline ? SKYBLUE : LIGHTGRAY);
        DrawRectangleLines(panelRect.x + 168, row2y, 60, 22, GRAY);
        DrawTextEx(headingFont, "API", {panelRect.x + 181, row2y + 2}, headingSize * 0.7f, 1, DARKGRAY);
    }

    float row3y = compositeProv ? (row1y + 70) : (row1y + 40);
    DrawTextEx(headingFont, "Color:", {panelRect.x + 10, row3y}, headingSize * 0.8f, 1, DARKGRAY);
    const auto& types = highlighter.getTypes();
    int activeId = highlighter.getActiveTypeId();
    for (size_t i = 0; i < types.size(); ++i) {
        float swatchX = panelRect.x + 30 + i * (SWATCH_SIZE + SWATCH_GAP);
        Color c = {types[i].color.r, types[i].color.g, types[i].color.b, 255};
        DrawRectangle(swatchX, row3y, SWATCH_SIZE, SWATCH_SIZE, c);
        if (types[i].id == activeId) {
            DrawRectangleLines(swatchX - 1, row3y - 1, SWATCH_SIZE + 2, SWATCH_SIZE + 2, BLACK);
        } else {
            DrawRectangleLines(swatchX, row3y, SWATCH_SIZE, SWATCH_SIZE, GRAY);
        }
    }
}

// ── Dialog Dispatch ────────────────────────────────────────────────────

void UIManager::dismissActiveDialog() {
    if (goToDialogActive) dismissGoToDialog();
    else if (settingsActive) dismissSettings();
    else if (contextMenuActive) hideContextMenu();
}
