#include "UIManager.h"
#include "RadialMenu.h"
#include "highlight/Highlighter.h"
#include <cmath>

namespace theword::renderer {

UIManager::UIManager(theword::highlight::Highlighter& highlighter, float dpiScale)
    : highlighter_(highlighter), dpiScale_(dpiScale) {}

UIManager::~UIManager() = default;

void UIManager::RecordDebugTap(Vector2 pos, bool wasHit) {
    debugTapPos_ = pos;
    debugTapTime_ = GetTime();
    debugTapWasHit_ = wasHit;
}

void UIManager::DrawDebugTap() {
    double elapsed = GetTime() - debugTapTime_;
    if (elapsed > 1.0) return;
    float alpha = 255.0f * (1.0f - static_cast<float>(elapsed));
    Color col = debugTapWasHit_ ? Color{255, 0, 0, static_cast<unsigned char>(alpha)}
                                : Color{255, 200, 0, static_cast<unsigned char>(alpha)};
    DrawCircleV(debugTapPos_, 5.0f * dpiScale_, col);
}

void UIManager::ShowRadialMenu(Vector2 position, int startWord, int endWord,
                               const std::string& bookId, int chapterNum) {
    RecordDebugTap(position, true);
    if (!radialMenu) {
        radialMenu = std::make_unique<RadialMenu>(dpiScale_);
    }
    radialMenu->Show(position, startWord, endWord, bookId, chapterNum, highlighter_.GetTypes());
}

void UIManager::HideRadialMenu() {
    if (radialMenu) radialMenu->Hide();
    highlighter_.ClearCommittedSelection();
}

bool UIManager::IsRadialMenuActive() const {
    return radialMenu && radialMenu->IsActive();
}

void UIManager::DrawRadialMenu() {
    if (radialMenu) radialMenu->Draw();
    // DrawDebugTap();
}

RadialMenuActionResult UIManager::HandleRadialMenuClick(Vector2 pos) {
    RecordDebugTap(pos, false);
    RadialMenuActionResult result;
    if (!radialMenu || !radialMenu->IsActive()) return result;

    auto [action, colorIndex] = radialMenu->HandleClick(pos);
    result.startWord = radialMenu->GetStartWord();
    result.endWord = radialMenu->GetEndWord();
    result.bookId = radialMenu->GetBookId();
    result.chapterNum = radialMenu->GetChapterNum();

    switch (action) {
        case RadialMenu::Action::Highlight:
            result.consumed = true;
            result.isHighlight = true;
            result.colorIndex = colorIndex;
            break;

        case RadialMenu::Action::Copy:
            result.consumed = true;
            result.isCopy = true;
            HideRadialMenu();
            break;

        case RadialMenu::Action::Delete:
            result.consumed = true;
            result.isDelete = true;
            HideRadialMenu();
            break;

        case RadialMenu::Action::None:
            result.consumed = false;
            break;
    }

    debugTapWasHit_ = result.consumed;
    return result;
}

} // namespace theword::renderer
