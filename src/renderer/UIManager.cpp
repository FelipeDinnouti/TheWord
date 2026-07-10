#include "UIManager.h"
#include "RadialMenu.h"
#include "highlight/Highlighter.h"

namespace theword::renderer {

UIManager::UIManager(theword::highlight::Highlighter& highlighter, float dpiScale)
    : highlighter_(highlighter), dpiScale_(dpiScale) {}

UIManager::~UIManager() = default;

void UIManager::ShowRadialMenu(Vector2 position, int startWord, int endWord,
                               const std::string& bookId, int chapterNum) {
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
}

RadialMenuActionResult UIManager::HandleRadialMenuClick(Vector2 pos) {
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

    return result;
}

} // namespace theword::renderer
