#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <memory>
#include <string>
#include <raylib.h>

namespace theword::highlight { class Highlighter; }

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
    UIManager(theword::highlight::Highlighter& highlighter, float dpiScale = 1.0f);

    ~UIManager();

    void DrawRadialMenu();
    RadialMenuActionResult HandleRadialMenuClick(Vector2 pos);
    void ShowRadialMenu(Vector2 position, int startWord, int endWord,
                        const std::string& bookId, int chapterNum);
    bool IsRadialMenuActive() const;
    void HideRadialMenu();

    void RecordDebugTap(Vector2 pos, bool wasHit = false);
    void DrawDebugTap();

private:
    std::unique_ptr<RadialMenu> radialMenu;
    theword::highlight::Highlighter& highlighter_;
    float dpiScale_;

    Vector2 debugTapPos_{0, 0};
    double debugTapTime_ = 0.0;
    bool debugTapWasHit_ = false;
};

} // namespace theword::renderer

#endif
