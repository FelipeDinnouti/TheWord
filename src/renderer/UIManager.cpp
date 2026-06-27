#include "UIManager.h"
#include "ContextMenu.h"
#include "event/EventBus.h"
#include "core/Theme.h"

namespace theword::renderer {

using namespace theword::core;

UIManager::UIManager(theword::event::EventBus& eventBus,
                     const Font& headingFont, float headingSize,
                     theword::highlight::Highlighter& highlighter,
                     float scaleFactor)
    : contextMenu(new ContextMenu(headingFont, headingSize, highlighter, scaleFactor)) {}

UIManager::~UIManager() {
    delete contextMenu;
}

void UIManager::ShowContextMenu(Vector2 position, int highlightId, int typeId) {
    contextMenu->Show(position, highlightId, typeId);
}

void UIManager::HideContextMenu() {
    contextMenu->Hide();
}

bool UIManager::IsContextMenuActive() const {
    return contextMenu->IsActive();
}

void UIManager::DrawContextMenu() {
    contextMenu->Draw();
}

} // namespace theword::renderer
