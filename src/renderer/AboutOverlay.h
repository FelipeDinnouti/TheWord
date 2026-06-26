#ifndef ABOUT_OVERLAY_H
#define ABOUT_OVERLAY_H

#include <raylib.h>

namespace theword::renderer {

class AboutOverlay {
public:
    AboutOverlay(const Font& headingFont, float headingSize, float scale);

    void Toggle();
    void Dismiss();
    bool IsActive() const;
    void Draw();
    bool HandleClick(Vector2 pos);

private:
    const Font& headingFont;
    float headingSize;
    float scale;

    bool active;

    static constexpr float ABOUT_WIDTH = 420.0f;
    static constexpr float ABOUT_HEIGHT = 160.0f;
    static constexpr float PADDING = 16.0f;
    static constexpr float CLOSE_BUTTON_SIZE = 24.0f;

    Rectangle GetCloseButtonRect() const;
    void DrawCloseButton() const;
    void DrawBackdrop() const;
};

} // namespace theword::renderer

#endif
