#ifndef SETTINGS_SCREEN_H
#define SETTINGS_SCREEN_H

#include "Screen.h"
#include "core/UIScale.h"
#include "core/Locale.h"
#include "TapDetector.h"
#include <raylib.h>

namespace theword::event { class EventBus; }
namespace theword::persistence { class PersistenceManager; }
namespace theword::ui { class NavigationStack; }

namespace theword::ui {

class SettingsScreen : public Screen {
public:
    SettingsScreen(const Font& font, float fontSize,
                   NavigationStack& navStack,
                   theword::event::EventBus& eventBus,
                   theword::persistence::PersistenceManager& persistence,
                   const theword::core::UIScale& uiScale,
                   float& currentFontSize, int& currentBibleId, bool& immersiveMode);
    void Draw() override;
    bool HandleInput(float deltaTime) override;
    const char* GetTitle() const override { return theword::core::Locale::Get("Settings"); }

private:
    const Font& font_;
    float fontSize_;
    NavigationStack& navStack_;
    theword::event::EventBus& eventBus_;
    theword::persistence::PersistenceManager& persistence_;
    const theword::core::UIScale& uiScale_;
    float& currentFontSize_;
    int& currentBibleId_;
    bool& immersiveMode_;

    void ChangeFontSize(float delta);

    TapDetector tapDetector_;
};

} // namespace theword::ui

#endif
