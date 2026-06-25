#ifndef UIMANAGER_H
#define UIMANAGER_H

#include <string>
#include <vector>
#include <raylib.h>
#include "core/Config.h"
#include "core/Theme.h"

class Highlighter;
class DocumentManager;
class LayoutEngine;
class Renderer;
class PersistenceManager;
class ChapterProvider;
class CompositeProvider;

class UIManager {
public:
    UIManager(const Font& headingFont, float headingSize, Highlighter& highlighter,
              DocumentManager& docManager, LayoutEngine& layoutEngine,
              Renderer& renderer, PersistenceManager& persistence,
              ChapterProvider& onlineProv, ChapterProvider& offlineProv,
              CompositeProvider* compositeProv, float initialFontSize = 24.0f,
              bool initialVersionOnline = false, float scaleFactor = 1.0f);

    float GetContentTop() const;
    float GetFontSize() const;

    void DrawTopBar(const std::string& chapterTitle);
    void DrawContextMenu();
    void DrawSettingsPanel();
    void DrawGoToDialog();

    void ShowContextMenu(Vector2 position, int highlightId, int typeId);
    void HideContextMenu();
    bool IsContextMenuActive() const;
    bool HandleContextMenuClick(Vector2 pos);

    void ToggleGoToDialog();
    void DismissGoToDialog();
    bool IsGoToDialogActive() const;
    void HandleGoToKeyboardInput();
    void HandleGoToClick(Vector2 pos);

    void ToggleAbout();
    void DismissAbout();
    bool IsAboutActive() const;
    void DrawAbout();
    void HandleAboutClick(Vector2 pos);

    void ToggleSettings();
    void DismissSettings();
    bool IsSettingsActive() const;
    void HandleSettingsClick(Vector2 pos);

    void ChangeFontSize(float delta);
    void DismissActiveDialog();

private:
    const Font& headingFont;
    float headingSize;
    Highlighter& highlighter;
    DocumentManager& docManager;
    LayoutEngine& layoutEngine;
    Renderer& renderer;
    PersistenceManager& persistence;
    ChapterProvider& onlineProv;
    ChapterProvider& offlineProv;
    CompositeProvider* compositeProv;

    float currentFontSize;
    float scale;
    bool versionOnline;

    // Context menu
    bool contextMenuActive;
    Vector2 contextMenuPos;
    int contextHighlightId;
    int contextHighlightTypeId;

    // Go-to dialog
    bool goToDialogActive;
    std::string goToInput;
    int goToSelection;
    bool goToError;

    // About
    bool aboutActive;

    // Settings
    bool settingsActive;

    // Constants
    static constexpr float TOP_BAR_HEIGHT = config::TOP_BAR_HEIGHT;
    static constexpr float MENU_WIDTH = 190.0f;
    static constexpr float MENU_HEIGHT = 32.0f;
    static constexpr float SWATCH_SIZE = 20.0f;
    static constexpr float SWATCH_GAP = 4.0f;
    static constexpr float MENU_PADDING = 4.0f;
    static constexpr float DELETE_WIDTH = 50.0f;
    static constexpr float LABEL_SWATCH_GAP = 8.0f;
    static constexpr float DIALOG_WIDTH = 300.0f;
    static constexpr float GO_TO_DIALOG_HEIGHT = 200.0f;
    static constexpr float SETTINGS_WIDTH = 260.0f;
    static constexpr float SETTINGS_HEIGHT = 180.0f;
    static constexpr int BACKDROP_ALPHA = 120;
    static constexpr float CLOSE_SIZE = 18.0f;
    static constexpr float CLOSE_MARGIN = 6.0f;
    static constexpr float ABOUT_WIDTH = 300.0f;
    static constexpr float ABOUT_HEIGHT = 200.0f;
    static constexpr float ABOUT_TEXT_LINE_H = 24.0f;
    static constexpr float ABOUT_FIRST_LINE_Y = 50.0f;

    // Settings panel layout
    static constexpr float SETTINGS_LABEL_X = 10.0f;
    static constexpr float SETTINGS_ROW1_Y = 40.0f;
    static constexpr float SETTINGS_ROW_GAP = 30.0f;
    static constexpr float COLOR_ROW_SPACER = 40.0f;
    static constexpr float COLOR_SWATCH_START = 60.0f;

    static constexpr float FONT_BTN_W = 28.0f;
    static constexpr float FONT_BTN_H = 22.0f;
    static constexpr float FONT_BTN_X = 120.0f;
    static constexpr float FONT_SIZE_X = 154.0f;
    static constexpr float FONT_PLUS_X = 180.0f;

    static constexpr float SRC_BTN_W = 60.0f;
    static constexpr float SRC_BTN_H = 22.0f;
    static constexpr float SRC_USFM_X = 100.0f;
    static constexpr float SRC_ONLINE_X = 168.0f;

    // Go-to dialog layout
    static constexpr float INPUT_BOX_H = 30.0f;
    static constexpr float INPUT_BOX_INSET = 4.0f;
    static constexpr float SUGGESTION_ITEM_H = 22.0f;
    static constexpr float SUGGESTION_LINE_H = 24.0f;

    void DrawBackdrop();
    void ApplyFontSize(float newSize);
    Rectangle GetGoToDialogRect() const;
    Rectangle GetSettingsPanelRect() const;
    Rectangle GetCloseButtonRect(Rectangle panelRect) const;
    Rectangle GetAboutRect() const;

    std::vector<int> GetSuggestions() const;
    std::string ParseGoToInput(const std::string& input) const;
    static bool StartsWithIgnoreCase(const std::string& str, const std::string& prefix);

    static std::string TryParseBookDotChapter(const std::string& input);
    static std::string TryParseFullNameThenChapter(const std::string& input);
    static std::string TryParseSpaceSeparated(const std::string& input);

    void DrawCloseButton(Rectangle panelRect);
    void DrawGoToSuggestions(Rectangle dlg, const std::vector<int>& suggestions);

    void DrawSettingsFontRow(Rectangle panel);
    void DrawSettingsSourceRow(Rectangle panel);
    void DrawSettingsColorRow(Rectangle panel);

    bool HandleSettingsClickOnTitleClose(Vector2 pos, Rectangle panel);
    bool HandleSettingsClickOnFontRow(Vector2 pos, Rectangle panel);
    bool HandleSettingsClickOnSourceRow(Vector2 pos, Rectangle panel);
    void HandleSettingsClickOnColorRow(Vector2 pos, Rectangle panel);
};

#endif
