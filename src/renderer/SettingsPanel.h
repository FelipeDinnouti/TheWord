#ifndef SETTINGS_PANEL_H
#define SETTINGS_PANEL_H

#include <raylib.h>

namespace theword::highlight { class Highlighter; }

namespace theword::renderer {

class SettingsPanel {
public:
    enum class Action { None, Dismiss, FontDecrease, FontIncrease, SourceOffline, SourceOnline };

    SettingsPanel(const Font& headingFont, float headingSize,
                  theword::highlight::Highlighter& highlighter, float scale);

    void Toggle();
    void Dismiss();
    bool IsActive() const;

    void Draw(float currentFontSize, bool versionOnline);
    Action HandleClick(Vector2 pos);

private:
    const Font& headingFont;
    float headingSize;
    theword::highlight::Highlighter& highlighter;
    float scale;
    bool active;

    static constexpr float SETTINGS_WIDTH = 260.0f;
    static constexpr float SETTINGS_HEIGHT = 180.0f;
    static constexpr float SETTINGS_LABEL_X = 10.0f;
    static constexpr float SETTINGS_ROW1_Y = 40.0f;
    static constexpr float SETTINGS_ROW_GAP = 30.0f;
    static constexpr float COLOR_ROW_SPACER = 40.0f;
    static constexpr float SWATCH_SIZE = 20.0f;
    static constexpr float SWATCH_GAP = 4.0f;
    static constexpr float COLOR_SWATCH_START = 60.0f;
    static constexpr float CLOSE_SIZE = 18.0f;
    static constexpr float CLOSE_MARGIN = 6.0f;
    static constexpr float FONT_BTN_W = 28.0f;
    static constexpr float FONT_BTN_H = 22.0f;
    static constexpr float FONT_BTN_X = 120.0f;
    static constexpr float FONT_SIZE_X = 154.0f;
    static constexpr float FONT_PLUS_X = 180.0f;
    static constexpr float SRC_BTN_W = 60.0f;
    static constexpr float SRC_BTN_H = 22.0f;
    static constexpr float SRC_USFM_X = 100.0f;
    static constexpr float SRC_ONLINE_X = 168.0f;

    Rectangle GetPanelRect() const;
    Rectangle GetCloseButtonRect(Rectangle panel) const;
    void DrawBackdrop() const;
    void DrawCloseButton(Rectangle panel) const;

    bool HitTestClose(Vector2 pos) const;
    bool HitTestFontDecrease(Vector2 pos) const;
    bool HitTestFontIncrease(Vector2 pos) const;
    bool HitTestSourceOffline(Vector2 pos) const;
    bool HitTestSourceOnline(Vector2 pos) const;
    void HandleColorClick(Vector2 pos) const;
};

} // namespace theword::renderer

#endif
