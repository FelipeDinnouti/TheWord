#ifndef CENTER_MENU_H
#define CENTER_MENU_H

#include "Screen.h"
#include <memory>
#include <raylib.h>

namespace theword::event { class EventBus; }
namespace theword::highlight { class Highlighter; }
namespace theword::persistence { class PersistenceManager; }
namespace theword::ui { class NavigationStack; }

namespace theword::ui {

class CenterMenu : public Screen {
public:
    CenterMenu(const Font& font, float fontSize,
               NavigationStack& navStack,
               theword::event::EventBus& eventBus,
               theword::highlight::Highlighter& highlighter,
               theword::persistence::PersistenceManager& persistence,
               float scale, float& currentFontSize, bool& versionOnline);
    void Draw() override;
    bool HandleInput(float deltaTime) override;
    const char* GetTitle() const override { return "Menu"; }
    bool IsOverlay() const override { return true; }

private:
    const Font& font_;
    float fontSize_;
    NavigationStack& navStack_;
    theword::event::EventBus& eventBus_;
    theword::highlight::Highlighter& highlighter_;
    theword::persistence::PersistenceManager& persistence_;
    float scale_;
    float& currentFontSize_;
    bool& versionOnline_;

    static constexpr float MENU_WIDTH = 280.0f;
    static constexpr float MENU_ITEM_HEIGHT = 44.0f;
    static constexpr float MENU_PADDING = 12.0f;

    void HandleAction(int action);
    static const char* ItemLabel(int idx);

    int selectedIndex_ = 0;
};

} // namespace theword::ui

#endif
