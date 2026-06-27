#ifndef CONTEXT_MENU_H
#define CONTEXT_MENU_H

#include <raylib.h>

namespace theword::highlight { class Highlighter; }

namespace theword::renderer {

class ContextMenu {
public:
    ContextMenu(const Font& headingFont, float headingSize, theword::highlight::Highlighter& highlighter,
                float scaleFactor = 1.0f);

    void Show(Vector2 position, int highlightId, int typeId);
    void Hide();
    bool IsActive() const;
    void Draw();
    bool HandleClick(Vector2 pos);

private:
    const Font& headingFont;
    float headingSize;
    theword::highlight::Highlighter& highlighter;
    float scale;

    bool active;
    Vector2 pos;
    int highlightId;
    int typeId;

    static constexpr float MENU_WIDTH = 190.0f;
    static constexpr float MENU_HEIGHT = 32.0f;
    static constexpr float SWATCH_SIZE = 20.0f;
    static constexpr float SWATCH_GAP = 4.0f;
    static constexpr float MENU_PADDING = 4.0f;
    static constexpr float DELETE_WIDTH = 50.0f;
    static constexpr float LABEL_SWATCH_GAP = 8.0f;
};

} // namespace theword::renderer

#endif
