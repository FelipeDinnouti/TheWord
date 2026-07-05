#include "ContextMenu.h"
#include "ui/components.h"
#include "highlight/Highlighter.h"
#include "core/Theme.h"
#include "core/Locale.h"

namespace theword::renderer {

using namespace theword::core;

ContextMenu::ContextMenu(const Font& headingFont, float headingSize,
                         theword::highlight::Highlighter& highlighter, float scaleFactor)
    : headingFont(headingFont), headingSize(headingSize), highlighter(highlighter),
      scale(scaleFactor),
      active(false), pos{0, 0}, highlightId(-1), typeId(-1) {}

void ContextMenu::Show(Vector2 position, int highlightId, int typeId) {
    active = true;
    pos = position;
    this->highlightId = highlightId;
    this->typeId = typeId;

    float screenW = (float)GetScreenWidth();
    float screenH = (float)GetScreenHeight();
    float mw = MENU_WIDTH * scale;
    float mh = MENU_HEIGHT * scale;
    if (pos.x + mw > screenW)
        pos.x = position.x - mw - 4.0f * scale;
    if (pos.y + mh > screenH)
        pos.y = screenH - mh - 4.0f * scale;
    if (pos.x < 0) pos.x = 4.0f * scale;
    if (pos.y < 60.0f * scale) pos.y = 60.0f * scale;
}

void ContextMenu::Hide() { active = false; }

bool ContextMenu::IsActive() const { return active; }

void ContextMenu::Draw() {
    if (!active) return;

    float mw = MENU_WIDTH * scale;
    float mh = MENU_HEIGHT * scale;
    float sw = SWATCH_SIZE * scale;
    float sg = SWATCH_GAP * scale;
    float mp = MENU_PADDING * scale;
    float dw = DELETE_WIDTH * scale;
    float lg = LABEL_SWATCH_GAP * scale;

    theword::ui::DrawPanel({pos.x, pos.y, mw, mh}, theme::PANEL_BG, theme::PANEL_BORDER);

    float x0 = pos.x + mp;
    float y0 = pos.y + mp;
    float contentH = mh - mp * 2;

    theword::ui::DrawButton({x0, y0, dw, contentH}, Locale::Get("Del"), headingFont, headingSize,
                            true, theme::UI_DELETE, theme::PANEL_BG);

    float swatchStartX = x0 + dw + lg;
    float swatchY = y0 + (contentH - sw) / 2.0f;
    const auto& types = highlighter.GetTypes();
    for (size_t i = 0; i < types.size(); ++i) {
        float swatchX = swatchStartX + i * (sw + sg);
        Color c = {types[i].color.r, types[i].color.g, types[i].color.b, 255};
        theword::ui::DrawColorSwatch({swatchX, swatchY, sw, sw}, c, false);
    }
}

bool ContextMenu::HandleClick(Vector2 clickPos) {
    if (!active) return false;

    float mw = MENU_WIDTH * scale;
    float mh = MENU_HEIGHT * scale;
    float sw = SWATCH_SIZE * scale;
    float sg = SWATCH_GAP * scale;
    float mp = MENU_PADDING * scale;
    float dw = DELETE_WIDTH * scale;
    float lg = LABEL_SWATCH_GAP * scale;

    Rectangle menuRect = {pos.x, pos.y, mw, mh};
    if (!CheckCollisionPointRec(clickPos, menuRect)) {
        Hide();
        return false;
    }

    float x0 = pos.x + mp;
    float y0 = pos.y + mp;

    Rectangle deleteRect = {x0, y0, dw, mh - mp * 2};
    if (CheckCollisionPointRec(clickPos, deleteRect)) {
        highlighter.RemoveHighlight(highlightId);
        Hide();
        return true;
    }

    float swatchStartX = x0 + dw + lg;
    const auto& types = highlighter.GetTypes();
    for (size_t i = 0; i < types.size(); ++i) {
        float swatchX = swatchStartX + i * (sw + sg);
        float swatchY = y0 + (mh - mp * 2 - sw) / 2.0f;
        if (CheckCollisionPointRec(clickPos, {swatchX, swatchY, sw, sw})) {
            highlighter.RecolorHighlight(highlightId, types[i].id);
            Hide();
            return true;
        }
    }

    Hide();
    return true;
}

} // namespace theword::renderer
