#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <string>
#include <raylib.h>

namespace theword::event { class EventBus; }
namespace theword::highlight { class Highlighter; }

namespace theword::renderer {

class ContextMenu;

class UIManager {
public:
    UIManager(theword::event::EventBus& eventBus,
              const Font& headingFont, float headingSize,
              theword::highlight::Highlighter& highlighter,
              float scaleFactor = 1.0f);

    ~UIManager();

    void ShowContextMenu(Vector2 position, int highlightId, int typeId);
    void HideContextMenu();
    bool IsContextMenuActive() const;
    void DrawContextMenu();
    bool HandleContextMenuClick(Vector2 pos);

private:
    ContextMenu* contextMenu;
};

} // namespace theword::renderer

#endif
