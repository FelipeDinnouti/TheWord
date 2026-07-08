#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <string>
#include <memory>
#include <raylib.h>

namespace theword::event { class EventBus; }
namespace theword::highlight { class Highlighter; struct HighlightType; }

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
};

class UIManager {
public:
    UIManager(theword::event::EventBus& eventBus,
              const Font& headingFont, float headingSize,
              theword::highlight::Highlighter& highlighter,
              float scaleFactor = 1.0f);

    ~UIManager();

    void DrawRadialMenu();
    RadialMenuActionResult HandleRadialMenuClick(Vector2 pos);
    void ShowRadialMenu(Vector2 position, int startWord, int endWord);
    bool IsRadialMenuActive() const;
    void HideRadialMenu();

private:
    std::unique_ptr<RadialMenu> radialMenu;
    theword::highlight::Highlighter& highlighter_;
};

} // namespace theword::renderer

#endif
