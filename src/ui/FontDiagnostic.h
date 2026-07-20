#ifndef FONT_DIAGNOSTIC_H
#define FONT_DIAGNOSTIC_H

#include "Screen.h"
#include "core/Locale.h"
#include "core/ThemeManager.h"
#include <raylib.h>
#include <vector>

namespace theword::ui { class NavigationStack; }

namespace theword::ui {

class FontDiagnostic : public Screen {
public:
    FontDiagnostic(const Font& bodyFont, const Font& headingFont,
                   const Font& largeFont, const Font& smallFont,
                   const Font& boldFont, float dpiScale,
                   NavigationStack& navStack,
                   const theword::core::ThemeManager& themeManager);
    void Draw() override;
    bool HandleInput(float deltaTime) override;
    const char* GetTitle() const override { return theword::core::Locale::Get("Font Diagnostic"); }

private:
    const Font& bodyFont_;
    const Font& headingFont_;
    const Font& largeFont_;
    const Font& smallFont_;
    const Font& boldFont_;
    NavigationStack& navStack_;
    const theword::core::ThemeManager& themeManager_;

    int scrollY_ = 0;
    float contentHeight_ = 0.0f;

    void DrawSample(float& y, const char* label, const Font& font, float renderSize,
                    const char* sample, Color color, int filterMode);
};

} // namespace theword::ui

#endif
