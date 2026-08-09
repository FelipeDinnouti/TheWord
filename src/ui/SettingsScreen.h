#ifndef SETTINGS_SCREEN_H
#define SETTINGS_SCREEN_H

#include "Screen.h"
#include "core/UIScale.h"
#include "core/Locale.h"
#include "core/ThemeManager.h"
#include "TapDetector.h"
#include <raylib.h>

namespace theword::event { class EventBus; }
namespace theword::persistence { class PersistenceManager; }
namespace theword::ui { class NavigationStack; }

namespace theword::ui {

class SettingsScreen : public Screen {
public:
    SettingsScreen(NavigationStack& navStack,
                   theword::event::EventBus& eventBus,
                   theword::persistence::PersistenceManager& persistence,
                   const theword::core::UIScale& uiScale,
                   float& currentFontSize, int& currentBibleId, bool& immersiveMode,
                   const theword::core::ThemeManager& themeManager);
    void Draw(theword::renderer::DrawContext& ctx) override;
    bool HandleInput(float deltaTime) override;
    const char* GetTitle() const override { return theword::core::Locale::Get("Settings"); }

private:
    NavigationStack& navStack_;
    theword::event::EventBus& eventBus_;
    theword::persistence::PersistenceManager& persistence_;
    const theword::core::UIScale& uiScale_;
    float& currentFontSize_;
    int& currentBibleId_;
    bool& immersiveMode_;
    const theword::core::ThemeManager& themeManager_;

    void ChangeFontSize(float delta);

    TapDetector tapDetector_;
};

} // namespace theword::ui

#endif
