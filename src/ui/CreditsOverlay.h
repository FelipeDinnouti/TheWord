#ifndef CREDITS_OVERLAY_H
#define CREDITS_OVERLAY_H

#include "Screen.h"
#include "core/UIScale.h"
#include <raylib.h>

namespace theword::ui { class NavigationStack; }

namespace theword::ui {

class CreditsOverlay : public Screen {
public:
    CreditsOverlay(const Font& font, float fontSize, NavigationStack& navStack,
                   const theword::core::UIScale& uiScale);
    void Draw() override;
    bool HandleInput(float deltaTime) override;
    const char* GetTitle() const override { return "Credits"; }
    bool IsOverlay() const override { return true; }

private:
    const Font& font_;
    float fontSize_;
    NavigationStack& navStack_;
    const theword::core::UIScale& uiScale_;
};

} // namespace theword::ui

#endif
