#ifndef UIMANAGER_H
#define UIMANAGER_H

#include <string>
#include <vector>
#include <raylib.h>

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
              bool initialVersionOnline = false);

    float getContentTop() const;
    float getFontSize() const;

    // Drawing
    void drawTopBar(const std::string& chapterTitle);
    void drawContextMenu();
    void drawSettingsPanel();
    void drawGoToDialog();

    // Context menu
    void showContextMenu(Vector2 position, int highlightId, int typeId);
    void hideContextMenu();
    bool isContextMenuActive() const;
    bool handleContextMenuClick(Vector2 pos);

    // Go-to dialog
    void toggleGoToDialog();
    void dismissGoToDialog();
    bool isGoToDialogActive() const;
    void handleGoToKeyboardInput();
    void handleGoToClick(Vector2 pos);

    // Settings panel
    void toggleSettings();
    void dismissSettings();
    bool isSettingsActive() const;
    void handleSettingsClick(Vector2 pos);

    void dismissActiveDialog();

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

    // Settings
    bool settingsActive;

    // Constants
    static constexpr float TOP_BAR_HEIGHT = 60.0f;
    static constexpr float CONTENT_PADDING = 40.0f;
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

    // Helpers
    void drawBackdrop();
    void applyFontSize(float newSize);
    Rectangle getGoToDialogRect() const;
    Rectangle getSettingsPanelRect() const;
    Rectangle getCloseButtonRect(Rectangle panelRect) const;

    std::vector<int> getSuggestions() const;
    std::string parseGoToInput(const std::string& input) const;
    static bool startsWithIgnoreCase(const std::string& str, const std::string& prefix);
};

#endif
