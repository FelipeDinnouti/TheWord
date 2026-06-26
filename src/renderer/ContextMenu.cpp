#include "ContextMenu.h"
#include "highlight/Highlighter.h"
#include "core/Theme.h"

namespace theword::renderer {

using namespace theword::core;

ContextMenu::ContextMenu(const Font& headingFont, float headingSize, theword::highlight::Highlighter& highlighter)
    : headingFont(headingFont), headingSize(headingSize), highlighter(highlighter),
      active(false), pos{0, 0}, highlightId(-1), typeId(-1) {}

void ContextMenu::Show(Vector2 position, int highlightId, int typeId) {
    active = true;
    pos = position;
    this->highlightId = highlightId;
    this->typeId = typeId;

    float screenW = (float)GetScreenWidth();
    float screenH = (float)GetScreenHeight();
    if (pos.x + MENU_WIDTH > screenW)
        pos.x = position.x - MENU_WIDTH - 4;
    if (pos.y + MENU_HEIGHT > screenH)
        pos.y = screenH - MENU_HEIGHT - 4;
    if (pos.x < 0) pos.x = 4;
    if (pos.y < TOP_BAR_HEIGHT) pos.y = TOP_BAR_HEIGHT;
}

void ContextMenu::Hide() { active = false; }

bool ContextMenu::IsActive() const { return active; }

void ContextMenu::Draw() {
    if (!active) return;

    DrawRectangle(pos.x, pos.y, MENU_WIDTH, MENU_HEIGHT, theme::PANEL_BG);
    DrawRectangleLines(pos.x, pos.y, MENU_WIDTH, MENU_HEIGHT, theme::PANEL_BORDER);

    float x0 = pos.x + MENU_PADDING;
    float y0 = pos.y + MENU_PADDING;
    float contentH = MENU_HEIGHT - MENU_PADDING * 2;

    DrawTextEx(headingFont, "Del", {x0, y0 + (contentH - headingSize) / 2.0f},
               headingSize, 1, theme::UI_DELETE);

    float swatchStartX = x0 + DELETE_WIDTH + LABEL_SWATCH_GAP;
    float swatchY = y0 + (contentH - SWATCH_SIZE) / 2.0f;
    const auto& types = highlighter.GetTypes();
    for (size_t i = 0; i < types.size(); ++i) {
        float swatchX = swatchStartX + i * (SWATCH_SIZE + SWATCH_GAP);
        Color c = {types[i].color.r, types[i].color.g, types[i].color.b, 255};
        DrawRectangle(swatchX, swatchY, SWATCH_SIZE, SWATCH_SIZE, c);
        DrawRectangleLines(swatchX, swatchY, SWATCH_SIZE, SWATCH_SIZE, theme::BUTTON_BORDER);
    }
}

bool ContextMenu::HandleClick(Vector2 clickPos) {
    if (!active) return false;

    Rectangle menuRect = {pos.x, pos.y, MENU_WIDTH, MENU_HEIGHT};
    if (!CheckCollisionPointRec(clickPos, menuRect)) {
        Hide();
        return false;
    }

    float x0 = pos.x + MENU_PADDING;
    float y0 = pos.y + MENU_PADDING;

    Rectangle deleteRect = {x0, y0, DELETE_WIDTH, MENU_HEIGHT - MENU_PADDING * 2};
    if (CheckCollisionPointRec(clickPos, deleteRect)) {
        highlighter.RemoveHighlight(highlightId);
        Hide();
        return true;
    }

    float swatchStartX = x0 + DELETE_WIDTH + LABEL_SWATCH_GAP;
    const auto& types = highlighter.GetTypes();
    for (size_t i = 0; i < types.size(); ++i) {
        float swatchX = swatchStartX + i * (SWATCH_SIZE + SWATCH_GAP);
        float swatchY = y0 + (MENU_HEIGHT - MENU_PADDING * 2 - SWATCH_SIZE) / 2.0f;
        if (CheckCollisionPointRec(clickPos, {swatchX, swatchY, SWATCH_SIZE, SWATCH_SIZE})) {
            highlighter.RecolorHighlight(highlightId, types[i].id);
            Hide();
            return true;
        }
    }

    Hide();
    return true;
}

} // namespace theword::renderer
