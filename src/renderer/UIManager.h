#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <string>
#include <raylib.h>

namespace theword::event {
    class EventBus;
    struct KeyEvent;
    struct DialogEvent;
    struct FontSizeEvent;
}

namespace theword::highlight { class Highlighter; }
namespace theword::persistence { class PersistenceManager; }

namespace theword::renderer {

class ContextMenu;
class SettingsPanel;
class GoToDialog;
class AboutOverlay;

class UIManager {
public:
    UIManager(theword::event::EventBus& eventBus,
              const Font& headingFont, float headingSize, theword::highlight::Highlighter& highlighter,
              theword::persistence::PersistenceManager& persistence,
              float initialFontSize = 24.0f,
              bool initialVersionOnline = false, float scaleFactor = 1.0f);

    ~UIManager();

    void UpdateActiveDialog();

    float GetContentTop() const;
    float GetFontSize() const;

    void DrawTopBar(const std::string& chapterTitle);
    void DrawContextMenu();
    void DrawSettingsPanel();
    void DrawGoToDialog();
    void DrawAbout();

    void ShowContextMenu(Vector2 position, int highlightId, int typeId);
    void HideContextMenu();
    bool IsContextMenuActive() const;

    void ToggleGoToDialog();
    void DismissGoToDialog();
    bool IsGoToDialogActive() const;

    void ToggleAbout();
    void DismissAbout();
    bool IsAboutActive() const;

    void ToggleSettings();
    void DismissSettings();
    bool IsSettingsActive() const;

    void ChangeFontSize(float delta);
    void OnFontSizeApplied(float newSize);
    void DismissActiveDialog();

private:
    theword::event::EventBus& eventBus_;
    const Font& headingFont;
    float headingSize;

    ContextMenu* contextMenu;
    SettingsPanel* settingsPanel;
    GoToDialog* goToDialog;
    AboutOverlay* aboutOverlay;

    theword::persistence::PersistenceManager& persistence;
    theword::highlight::Highlighter& highlighter;

    float currentFontSize;
    float scale;
    bool versionOnline;

    static constexpr float TOP_BAR_HEIGHT = 60.0f;

    void OnKey(const theword::event::KeyEvent& e);
    void OnDialog(const theword::event::DialogEvent& e);
    void OnFontSize(const theword::event::FontSizeEvent& e);
    void SwitchSource(bool online);
};

} // namespace theword::renderer

#endif
