#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <memory>
#include <string>
#include <raylib.h>

namespace theword::highlight { class Highlighter; }
namespace theword::core { class ThemeManager; }

namespace theword::renderer {

class RadialMenu;

struct RadialMenuActionResult {
    bool consumed = false;
    bool isHighlight = false;
    bool isCopy = false;
    bool isDelete = false;
    int colorIndex = -1;
    int startWord = -1;
    int endWord = -1;
    std::string bookId;
    int chapterNum = 0;
};

class UIManager {
public:
    UIManager(theword::highlight::Highlighter& highlighter, const Font& uiFont,
              const theword::core::ThemeManager& themeManager, float dpiScale = 1.0f);

    ~UIManager();

    void DrawRadialMenu();
    RadialMenuActionResult HandleRadialMenuClick(Vector2 pos);
    void ShowRadialMenu(Vector2 position, int startWord, int endWord,
                        const std::string& bookId, int chapterNum);
    bool IsRadialMenuActive() const;
    void HideRadialMenu();

    void ShowToast(const std::string& text);
    void DrawToast();

    void ShowFootnotePopup(const std::string& text, Vector2 position);
    void DrawFootnotePopup();
    void HideFootnotePopup();
    bool IsFootnotePopupActive() const;

private:
    std::unique_ptr<RadialMenu> radialMenu;
    theword::highlight::Highlighter& highlighter_;
    const Font& uiFont_;
    const theword::core::ThemeManager& themeManager_;
    float dpiScale_;

    std::string toastText_;
    double toastStartTime_ = 0.0;
    static constexpr double TOAST_DURATION = 1.5;

    std::string footnoteText_;
    Vector2 footnotePos_{0, 0};
    bool footnotePopupActive_ = false;

};

} // namespace theword::renderer

#endif
