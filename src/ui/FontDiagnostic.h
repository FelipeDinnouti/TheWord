#ifndef FONT_DIAGNOSTIC_H
#define FONT_DIAGNOSTIC_H

#include "Screen.h"
#include <raylib.h>

namespace theword::ui { class NavigationStack; }

namespace theword::ui {

class FontDiagnostic : public Screen {
public:
    FontDiagnostic(const Font& bodyFont, const Font& headingFont,
                   const Font& largeFont, const Font& smallFont,
                   const Font& boldFont,
                   float dpiScale, NavigationStack& navStack);
    void Draw() override;
    bool HandleInput(float deltaTime) override;
    const char* GetTitle() const override { return "FontDiagnostic"; }
    bool IsOverlay() const override { return true; }

private:
    const Font& bodyFont_;
    const Font& headingFont_;
    const Font& largeFont_;
    const Font& smallFont_;
    const Font& boldFont_;
    float dpiScale_;
    NavigationStack& navStack_;
    float scrollY_ = 0.0f;
    float contentHeight_ = 0.0f;

    void DrawSample(float& y, const char* label,
                    const Font& font, float renderSize,
                    const char* sample, Color color, int filterMode);
};

} // namespace theword::ui

#endif
