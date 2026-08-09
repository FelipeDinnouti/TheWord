#ifndef CREDITS_OVERLAY_H
#define CREDITS_OVERLAY_H

#include "Screen.h"
#include "core/UIScale.h"
#include "core/Locale.h"
#include "TapDetector.h"
#include <raylib.h>

namespace theword::ui { class NavigationStack; }

namespace theword::ui {

class CreditsOverlay : public Screen {
public:
    CreditsOverlay(float fontSize,
                   NavigationStack& navStack,
                   const theword::core::UIScale& uiScale);
    void Draw(theword::renderer::DrawContext& ctx) override;
    bool HandleInput(float deltaTime) override;
    const char* GetTitle() const override { return theword::core::Locale::Get("Credits"); }
    bool IsOverlay() const override { return true; }

private:
    float fontSize_;
    NavigationStack& navStack_;
    const theword::core::UIScale& uiScale_;

    double showTime_;
    double fadeOutStartTime_ = 0;
    bool fadingOut_ = false;
    bool popPending_ = false;
    static constexpr float FADE_DURATION = 0.1f;

    TapDetector tapDetector_;
};

} // namespace theword::ui

#endif
