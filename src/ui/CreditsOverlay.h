#ifndef CREDITS_OVERLAY_H
#define CREDITS_OVERLAY_H

#include "Screen.h"
#include <raylib.h>

namespace theword::ui { class NavigationStack; }

namespace theword::ui {

class CreditsOverlay : public Screen {
public:
    CreditsOverlay(const Font& font, float fontSize, NavigationStack& navStack);
    void Draw() override;
    bool HandleInput(float deltaTime) override;
    const char* GetTitle() const override { return "Credits"; }
    bool IsOverlay() const override { return true; }

private:
    const Font& font_;
    float fontSize_;
    NavigationStack& navStack_;

    static constexpr float PANEL_WIDTH = 360.0f;
    static constexpr float PANEL_HEIGHT = 200.0f;
    static constexpr float PADDING = 20.0f;
};

} // namespace theword::ui

#endif
