#ifndef SETTINGS_SCREEN_H
#define SETTINGS_SCREEN_H

#include "Screen.h"
#include "core/UIScale.h"
#include "core/Locale.h"
#include <raylib.h>

namespace theword::event { class EventBus; }
namespace theword::highlight { class Highlighter; }
namespace theword::persistence { class PersistenceManager; }
namespace theword::ui { class NavigationStack; }

namespace theword::ui {

class SettingsScreen : public Screen {
public:
    SettingsScreen(const Font& font, float fontSize,
                   NavigationStack& navStack,
                   theword::event::EventBus& eventBus,
                   theword::highlight::Highlighter& highlighter,
                   theword::persistence::PersistenceManager& persistence,
                   const theword::core::UIScale& uiScale,
                   float& currentFontSize, bool& versionOnline);
    void Draw() override;
    bool HandleInput(float deltaTime) override;
    const char* GetTitle() const override { return theword::core::Locale::Get("Settings"); }

private:
    const Font& font_;
    float fontSize_;
    NavigationStack& navStack_;
    theword::event::EventBus& eventBus_;
    theword::highlight::Highlighter& highlighter_;
    theword::persistence::PersistenceManager& persistence_;
    const theword::core::UIScale& uiScale_;
    float& currentFontSize_;
    bool& versionOnline_;

    void ChangeFontSize(float delta);
    void SwitchSource(bool online);

    Vector2 pressStartPos_{};
    bool hasPendingPress_ = false;
};

} // namespace theword::ui

#endif
