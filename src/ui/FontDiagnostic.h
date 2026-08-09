#ifndef FONT_DIAGNOSTIC_H
#define FONT_DIAGNOSTIC_H

#include "Screen.h"
#include "core/Locale.h"
#include <raylib.h>
#include <vector>

namespace theword::ui { class NavigationStack; }

namespace theword::ui {

class FontDiagnostic : public Screen {
public:
    explicit FontDiagnostic(NavigationStack& navStack);
    void Draw(theword::renderer::DrawContext& ctx) override;
    bool HandleInput(float deltaTime) override;
    const char* GetTitle() const override { return theword::core::Locale::Get("Font Diagnostic"); }

private:
    NavigationStack& navStack_;

    int scrollY_ = 0;
    float contentHeight_ = 0.0f;

    void DrawSample(const theword::core::ThemePalette& pal, float& y, const char* label,
                    const Font& font, float renderSize, const char* sample, Color color,
                    int filterMode);
};

} // namespace theword::ui

#endif
