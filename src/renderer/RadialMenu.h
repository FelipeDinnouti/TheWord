#ifndef RADIAL_MENU_H
#define RADIAL_MENU_H

#include <raylib.h>
#include <vector>
#include <string>
#include <utility>

namespace theword::highlight { struct HighlightType; }

namespace theword::renderer {

class RadialMenu {
public:
    enum class Action { None, Copy, Delete, Highlight };

    RadialMenu();

    void Show(Vector2 center, int startWord, int endWord,
              const std::vector<theword::highlight::HighlightType>& types);
    void Hide();
    bool IsActive() const;
    void Draw();
    std::pair<Action, int> HandleClick(Vector2 pos);

    int GetStartWord() const { return startWord_; }
    int GetEndWord() const { return endWord_; }

private:
    struct Button {
        Vector2 center;
        float radius;
        Action action;
        int colorIndex = -1;
        Color fillColor;
        Color borderColor;
        std::string label;
    };

    Vector2 center_;
    int startWord_;
    int endWord_;
    bool active_;
    std::vector<Button> buttons_;
    int hoveredIndex_;

    void UpdateHover(Vector2 mousePos);
    void LayoutButtons(const std::vector<theword::highlight::HighlightType>& types);
};

} // namespace theword::renderer

#endif
