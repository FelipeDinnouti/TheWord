#include "CenterMenu.h"
#include "BookListScreen.h"
#include "SettingsScreen.h"
#include "CreditsOverlay.h"
#include "NavigationStack.h"
#include "core/Theme.h"
#include "core/Config.h"
#include <cmath>

namespace theword::ui {

using namespace theword::core;

static constexpr int ITEM_COUNT = 4;

static const char* ITEM_LABELS[ITEM_COUNT] = {
    "Books",
    "Settings",
    "Highlights",
    "Credits"
};

const char* CenterMenu::ItemLabel(int idx) {
    if (idx >= 0 && idx < ITEM_COUNT) return ITEM_LABELS[idx];
    return "";
}

CenterMenu::CenterMenu(const Font& font, float fontSize,
                       NavigationStack& navStack,
                       theword::event::EventBus& eventBus,
                       theword::highlight::Highlighter& highlighter,
                       theword::persistence::PersistenceManager& persistence,
                       const theword::core::UIScale& uiScale,
                       float& currentFontSize, bool& versionOnline)
    : font_(font), fontSize_(fontSize),
      navStack_(navStack), eventBus_(eventBus),
      highlighter_(highlighter), persistence_(persistence),
      uiScale_(uiScale), currentFontSize_(currentFontSize), versionOnline_(versionOnline) {}

void CenterMenu::Draw() {
    float screenW = static_cast<float>(GetScreenWidth());
    float screenH = static_cast<float>(GetScreenHeight());

    DrawRectangle(0, 0, static_cast<int>(screenW), static_cast<int>(screenH), theme::OVERLAY_BG);

    float menuW = uiScale_.fitScreen(85, 320);
    float padding = uiScale_.dp(12);
    float itemH = std::max(uiScale_.dp(48), fontSize_ * 0.7f + uiScale_.dp(12));
    float totalHeight = ITEM_COUNT * itemH + padding * 2;
    float panelX = (screenW - menuW) / 2.0f;
    float panelY = (screenH - totalHeight) / 2.0f;

    DrawRectangle(static_cast<int>(panelX), static_cast<int>(panelY),
                  static_cast<int>(menuW), static_cast<int>(totalHeight), theme::PANEL_BG);
    DrawRectangleLines(static_cast<int>(panelX), static_cast<int>(panelY),
                       static_cast<int>(menuW), static_cast<int>(totalHeight),
                       theme::PANEL_BORDER);

    float itemY = panelY + padding;
    float labelSize = fontSize_ * 0.7f;

    for (int i = 0; i < ITEM_COUNT; ++i) {
        if (i == selectedIndex_) {
            DrawRectangle(static_cast<int>(panelX + padding),
                          static_cast<int>(itemY),
                          static_cast<int>(menuW - padding * 2),
                          static_cast<int>(itemH), theme::SELECTED_BG);
        }

        float textX = panelX + (menuW - MeasureTextEx(font_, ItemLabel(i), labelSize, 1).x) / 2.0f;
        float textY = itemY + (itemH - labelSize) / 2.0f;
        DrawTextEx(font_, ItemLabel(i), {textX, textY}, labelSize, 1, theme::UI_TEXT);

        if (i < ITEM_COUNT - 1) {
            DrawRectangle(static_cast<int>(panelX + padding),
                          static_cast<int>(itemY + itemH - 1),
                          static_cast<int>(menuW - padding * 2), 1, LIGHTGRAY);
        }
        itemY += itemH;
    }
}

bool CenterMenu::HandleInput(float /*deltaTime*/) {
    if (IsKeyPressed(key::ESCAPE)) {
        navStack_.Pop();
        return true;
    }

    if (IsKeyPressed(key::UP)) {
        selectedIndex_ = (selectedIndex_ - 1 + ITEM_COUNT) % ITEM_COUNT;
        return true;
    }

    if (IsKeyPressed(key::DOWN)) {
        selectedIndex_ = (selectedIndex_ + 1) % ITEM_COUNT;
        return true;
    }

    if (IsKeyPressed(key::ENTER)) {
        HandleAction(selectedIndex_);
        return true;
    }

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 mousePos = GetMousePosition();
        float screenW = static_cast<float>(GetScreenWidth());
        float screenH = static_cast<float>(GetScreenHeight());

        float menuW = uiScale_.fitScreen(85, 320);
        float padding = uiScale_.dp(12);
        float itemH = std::max(uiScale_.dp(48), fontSize_ * 0.7f + uiScale_.dp(12));
        float totalHeight = ITEM_COUNT * itemH + padding * 2;
        float panelX = (screenW - menuW) / 2.0f;
        float panelY = (screenH - totalHeight) / 2.0f;
        Rectangle panelRect = {panelX, panelY, menuW, totalHeight};

        if (!CheckCollisionPointRec(mousePos, panelRect)) {
            navStack_.Pop();
            return true;
        }

        float itemY = panelY + padding;
        for (int i = 0; i < ITEM_COUNT; ++i) {
            Rectangle itemRect = {panelX + padding, itemY,
                                  menuW - padding * 2, itemH};
            if (CheckCollisionPointRec(mousePos, itemRect)) {
                HandleAction(i);
                return true;
            }
            itemY += itemH;
        }
    }

    return false;
}

void CenterMenu::HandleAction(int action) {
    switch (action) {
        case 0: // Books
            navStack_.Push(std::make_unique<BookListScreen>(
                font_, fontSize_, navStack_, eventBus_, uiScale_
            ));
            return;
        case 1: // Settings
            navStack_.Push(std::make_unique<SettingsScreen>(
                font_, fontSize_, navStack_, eventBus_,
                highlighter_, persistence_,
                uiScale_, currentFontSize_, versionOnline_
            ));
            return;
        case 2: // Highlights (Phase 13)
            break;
        case 3: // Credits
            navStack_.Push(std::make_unique<CreditsOverlay>(
                font_, fontSize_, navStack_, uiScale_
            ));
            return;
    }
    navStack_.Pop();
}

} // namespace theword::ui
