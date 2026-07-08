#include "UIManager.h"
#include "RadialMenu.h"
#include "event/EventBus.h"
#include "highlight/Highlighter.h"
#include "core/Theme.h"

namespace theword::renderer {

using namespace theword::core;

UIManager::UIManager(theword::event::EventBus& /*eventBus*/,
                     const Font& /*headingFont*/, float /*headingSize*/,
                     theword::highlight::Highlighter& highlighter,
                     float /*scaleFactor*/)
    : highlighter_(highlighter) {}

UIManager::~UIManager() = default;

void UIManager::ShowRadialMenu(Vector2 position, int startWord, int endWord) {
    if (!radialMenu) {
        radialMenu = std::make_unique<RadialMenu>();
    }
    radialMenu->Show(position, startWord, endWord, highlighter_.GetTypes());
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

    switch (action) {
        case RadialMenu::Action::Highlight:
            result.consumed = true;
            result.isHighlight = true;
            result.colorIndex = colorIndex;
            {
                const auto& types = highlighter_.GetTypes();
                if (colorIndex >= 0 && colorIndex < static_cast<int>(types.size())) {
                    highlighter_.CreateHighlight(
                        result.startWord, result.endWord, types[colorIndex].id);
                }
            }
            HideRadialMenu();
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
            result.consumed = true;
            HideRadialMenu();
            break;
    }

    return result;
}

} // namespace theword::renderer
