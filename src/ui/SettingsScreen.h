#ifndef SETTINGS_SCREEN_H
#define SETTINGS_SCREEN_H

#include "Screen.h"
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
                   float scale, float& currentFontSize, bool& versionOnline);
    void Draw() override;
    bool HandleInput(float deltaTime) override;
    const char* GetTitle() const override { return "Settings"; }

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

    static constexpr float HEADER_HEIGHT = 60.0f;
    static constexpr float ROW_Y1 = 100.0f;
    static constexpr float ROW_GAP = 50.0f;
    static constexpr float LABEL_X = 30.0f;
    static constexpr float FONT_BTN_W = 36.0f;
    static constexpr float FONT_BTN_H = 30.0f;
    static constexpr float FONT_DEC_X = 140.0f;
    static constexpr float FONT_VAL_X = 190.0f;
    static constexpr float FONT_INC_X = 240.0f;
    static constexpr float SRC_BTN_W = 80.0f;
    static constexpr float SRC_BTN_H = 30.0f;
    static constexpr float SRC_USFM_X = 140.0f;
    static constexpr float SRC_ONLINE_X = 230.0f;
    static constexpr float SWATCH_SIZE = 28.0f;
    static constexpr float SWATCH_GAP = 8.0f;
    static constexpr float COLOR_START_X = 140.0f;
    static constexpr float BACK_AREA_WIDTH = 80.0f;

    void ChangeFontSize(float delta);
    void SwitchSource(bool online);
};

} // namespace theword::ui

#endif
