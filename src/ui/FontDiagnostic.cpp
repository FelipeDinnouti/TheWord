#include "FontDiagnostic.h"
#include "NavigationStack.h"
#include "core/Config.h"
#include "core/Locale.h"
#include <cstdio>

namespace theword::ui {

using namespace theword::core;

FontDiagnostic::FontDiagnostic(NavigationStack& navStack)
    : navStack_(navStack) {}

static void SetFilter(Font& font, int mode) {
    SetTextureFilter(font.texture, mode);
}

static void DrawTextAt(const Font& font, const char* text,
                       float x, float y, float size, Color color) {
    DrawTextEx(font, text, {x, y}, size, 1, color);
}

void FontDiagnostic::DrawSample(const theword::core::ThemePalette& pal, float& y,
                                const char* label, const Font& font, float renderSize,
                                const char* sample, Color color, int filterMode) {
    float baseSize = (float)font.baseSize;
    float ratio = renderSize / baseSize;

    float labelSize = renderSize * 0.45f;
    if (labelSize < 7.0f) labelSize = 7.0f;

    char info[64];
    std::snprintf(info, sizeof(info), "%s  %s  (a%d r%.0f r%.2f)",
                  label,
                  filterMode == TEXTURE_FILTER_POINT ? "P" : "B",
                  font.baseSize, renderSize, ratio);

    DrawTextAt(font, info, 10.0f, y + scrollY_, labelSize, pal.uiText);
    y += labelSize + 4.0f;

    DrawTextAt(font, sample, 10.0f, y + scrollY_, renderSize, color);
    y += renderSize + 14.0f;
}

void FontDiagnostic::Draw(theword::renderer::DrawContext& ctx) {
    const auto& pal = ctx.themeManager.Current();
    float screenW = ctx.uiScale.screenW;
    float screenH = ctx.uiScale.screenH;

    Font bodyFont = ctx.fonts.Get(theword::text::FontKind::Body);
    Font headingFont = ctx.fonts.Get(theword::text::FontKind::Heading);
    Font largeFont = ctx.fonts.Get(theword::text::FontKind::Large);
    Font smallFont = ctx.fonts.Get(theword::text::FontKind::Small);
    Font boldFont = ctx.fonts.Bold();

    DrawRectangle(0, 0, static_cast<int>(screenW), static_cast<int>(screenH), pal.windowBg);

    // Clipping region for scrollable content (leave room for header + footer)
    float clipY = 30.0f;
    BeginScissorMode(0, static_cast<int>(clipY), static_cast<int>(screenW),
                     static_cast<int>(screenH - clipY - 24.0f));

    float bodySz = (float)bodyFont.baseSize;
    float headingSz = (float)headingFont.baseSize;
    float largeSz = (float)largeFont.baseSize;
    float smallSz = (float)smallFont.baseSize;
    float boldSz = (float)boldFont.baseSize;
    contentHeight_ = 36.0f + bodySz * 0.5f + 12.0f + bodySz * 0.5f + 12.0f;

    float sizes[] = {bodySz, headingSz, largeSz, smallSz, bodySz, headingSz, largeSz, smallSz};
    for (int i = 0; i < 8; i++) {
        float s = sizes[i % 4];
        contentHeight_ += s * 0.45f + 4.0f + s + 14.0f;
    }
    // bold section
    contentHeight_ += bodySz * 0.5f + 12.0f + bodySz * 0.45f + 4.0f + boldSz + 14.0f + bodySz * 0.45f + 4.0f + boldSz + 14.0f;
    contentHeight_ += 60.0f;

    // Clamp scroll
    float maxScroll = contentHeight_ - (screenH - clipY - 24.0f);
    if (maxScroll < 0.0f) maxScroll = 0.0f;
    if (scrollY_ > 0.0f) scrollY_ = 0.0f;
    if (-scrollY_ > maxScroll) scrollY_ = -maxScroll;

    float y = 36.0f;

    // Font copies for SetTextureFilter
    Font bodyMut = bodyFont;
    Font headingMut = headingFont;
    Font largeMut = largeFont;
    Font smallMut = smallFont;

    // --- POINT samples ---
    DrawTextAt(bodyMut, Locale::Get("--- POINT ---"), 10.0f, y + scrollY_, bodySz * 0.5f, pal.uiTitle);
    y += bodySz * 0.5f + 12.0f;
    SetFilter(bodyMut, TEXTURE_FILTER_POINT);
    SetFilter(headingMut, TEXTURE_FILTER_POINT);
    SetFilter(largeMut, TEXTURE_FILTER_POINT);
    SetFilter(smallMut, TEXTURE_FILTER_POINT);

    DrawSample(pal, y, Locale::Get("body"),
               bodyMut, bodySz,
               "The quick brown fox jumps over the lazy dog.", pal.docBody, TEXTURE_FILTER_POINT);

    DrawSample(pal, y, Locale::Get("heading"),
               headingMut, headingSz,
               "The Quick Brown Fox Jumps", pal.docBody, TEXTURE_FILTER_POINT);

    DrawSample(pal, y, Locale::Get("large"),
               largeMut, largeSz,
               "The Gospel According to John", pal.docBody, TEXTURE_FILTER_POINT);

    DrawSample(pal, y, Locale::Get("small"),
               smallMut, smallSz,
               "1 2 3 4 5 6 7 8 9 10 11 12 13 14 15", pal.docBody, TEXTURE_FILTER_POINT);

    // --- BILINEAR comparison ---
    DrawTextAt(bodyMut, "--- BILINEAR ---", 10.0f, y + scrollY_, bodySz * 0.5f, pal.uiTitle);
    y += bodySz * 0.5f + 12.0f;
    SetFilter(bodyMut, TEXTURE_FILTER_BILINEAR);
    SetFilter(headingMut, TEXTURE_FILTER_BILINEAR);
    SetFilter(largeMut, TEXTURE_FILTER_BILINEAR);
    SetFilter(smallMut, TEXTURE_FILTER_BILINEAR);

    DrawSample(pal, y, Locale::Get("body"),
               bodyMut, bodySz,
               "The quick brown fox jumps over the lazy dog.", pal.docBody, TEXTURE_FILTER_BILINEAR);

    DrawSample(pal, y, Locale::Get("heading"),
               headingMut, headingSz,
               "The Quick Brown Fox Jumps", pal.docBody, TEXTURE_FILTER_BILINEAR);

    DrawSample(pal, y, Locale::Get("large"),
               largeMut, largeSz,
               "The Gospel According to John", pal.docBody, TEXTURE_FILTER_BILINEAR);

    DrawSample(pal, y, Locale::Get("small"),
               smallMut, smallSz,
               "1 2 3 4 5 6 7 8 9 10 11 12 13 14 15", pal.docBody, TEXTURE_FILTER_BILINEAR);

    // --- Bold comparison (bold font, POINT) ---
    DrawTextAt(bodyMut, Locale::Get("--- BOLD (POINT) ---"), 10.0f, y + scrollY_, bodySz * 0.5f, pal.uiTitle);
    y += bodySz * 0.5f + 12.0f;
    Font boldMut = boldFont;
    SetFilter(boldMut, TEXTURE_FILTER_POINT);
    DrawSample(pal, y, Locale::Get("body bold"),
               boldMut, boldSz,
               "The quick brown fox jumps over the lazy dog.", pal.docBody, TEXTURE_FILTER_POINT);

    DrawSample(pal, y, Locale::Get("body bold"),
               boldMut, boldSz,
               "The Quick Brown Fox Jumps In Bold", pal.docBody, TEXTURE_FILTER_POINT);

    // Restore — body/large/small stay POINT, heading stays BILINEAR
    SetFilter(bodyMut, TEXTURE_FILTER_POINT);
    SetFilter(headingMut, TEXTURE_FILTER_BILINEAR);
    SetFilter(largeMut, TEXTURE_FILTER_POINT);
    SetFilter(smallMut, TEXTURE_FILTER_POINT);

    EndScissorMode();

    // --- Non-scrolling header & footer ---
    DrawTextAt(GetFontDefault(), Locale::Get("Font Diagnostic"), 10.0f, 8.0f, 14.0f, pal.uiTitle);

    char footer[128];
    std::snprintf(footer, sizeof(footer),
                  "Body atlas=%dpx  Heading=%dpx  Large=%dpx  Small=%dpx  |  %s",
                  bodyFont.baseSize, headingFont.baseSize,
                  largeFont.baseSize, smallFont.baseSize,
                  Locale::Get("ESC to close"));
    DrawTextAt(GetFontDefault(), footer, 10.0f, screenH - 20.0f, 12.0f, pal.uiText);
}

bool FontDiagnostic::HandleInput(float /*deltaTime*/) {
    if (IsKeyPressed(key::ESCAPE)) {
        navStack_.Pop();
        return true;
    }

    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) {
        scrollY_ += wheel * 40.0f;
        return true;
    }

    return false;
}

} // namespace theword::ui
