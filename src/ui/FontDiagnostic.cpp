#include "FontDiagnostic.h"
#include "NavigationStack.h"
#include "core/Theme.h"
#include "core/Config.h"
#include <cstdio>
#include <cstring>

namespace theword::ui {

using namespace theword::core;

FontDiagnostic::FontDiagnostic(const Font& bodyFont, const Font& headingFont,
                               const Font& largeFont, const Font& smallFont,
                               const Font& boldFont,
                               float dpiScale, NavigationStack& navStack)
    : bodyFont_(bodyFont), headingFont_(headingFont),
      largeFont_(largeFont), smallFont_(smallFont), boldFont_(boldFont),
      dpiScale_(dpiScale), navStack_(navStack) {}

static void SetFilter(Font& font, int mode) {
    SetTextureFilter(font.texture, mode);
}

static void DrawTextAt(const Font& font, const char* text,
                       float x, float y, float size, Color color) {
    DrawTextEx(font, text, {x, y}, size, 1, color);
}

void FontDiagnostic::DrawSample(float& y, const char* label,
                                const Font& font, float renderSize,
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

    DrawTextAt(font, info, 10.0f, y + scrollY_, labelSize, GRAY);
    y += labelSize + 4.0f;

    DrawTextAt(font, sample, 10.0f, y + scrollY_, renderSize, color);
    y += renderSize + 14.0f;
}

void FontDiagnostic::Draw() {
    float screenW = static_cast<float>(GetScreenWidth());
    float screenH = static_cast<float>(GetScreenHeight());

    DrawRectangle(0, 0, static_cast<int>(screenW), static_cast<int>(screenH), WHITE);

    // Clipping region for scrollable content (leave room for header + footer)
    float clipY = 30.0f;
    BeginScissorMode(0, static_cast<int>(clipY), static_cast<int>(screenW),
                     static_cast<int>(screenH - clipY - 24.0f));

    float bodySz = (float)bodyFont_.baseSize;
    float headingSz = (float)headingFont_.baseSize;
    float largeSz = (float)largeFont_.baseSize;
    float smallSz = (float)smallFont_.baseSize;
    float boldSz = (float)boldFont_.baseSize;
    Color docColor = {50, 50, 50, 255};
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
    Font bodyMut = bodyFont_;
    Font headingMut = headingFont_;
    Font largeMut = largeFont_;
    Font smallMut = smallFont_;

    // ─── POINT samples ───
    DrawTextAt(bodyMut, "--- POINT ---", 10.0f, y + scrollY_, bodySz * 0.5f, DARKGRAY);
    y += bodySz * 0.5f + 12.0f;
    SetFilter(bodyMut, TEXTURE_FILTER_POINT);
    SetFilter(headingMut, TEXTURE_FILTER_POINT);
    SetFilter(largeMut, TEXTURE_FILTER_POINT);
    SetFilter(smallMut, TEXTURE_FILTER_POINT);

    DrawSample(y, "body",
               bodyMut, bodySz,
               "The quick brown fox jumps over the lazy dog.", docColor, TEXTURE_FILTER_POINT);

    DrawSample(y, "heading",
               headingMut, headingSz,
               "The Quick Brown Fox Jumps", docColor, TEXTURE_FILTER_POINT);

    DrawSample(y, "large",
               largeMut, largeSz,
               "The Gospel According to John", docColor, TEXTURE_FILTER_POINT);

    DrawSample(y, "small",
               smallMut, smallSz,
               "1 2 3 4 5 6 7 8 9 10 11 12 13 14 15", docColor, TEXTURE_FILTER_POINT);

    // ─── BILINEAR comparison ───
    DrawTextAt(bodyMut, "--- BILINEAR ---", 10.0f, y + scrollY_, bodySz * 0.5f, DARKGRAY);
    y += bodySz * 0.5f + 12.0f;
    SetFilter(bodyMut, TEXTURE_FILTER_BILINEAR);
    SetFilter(headingMut, TEXTURE_FILTER_BILINEAR);
    SetFilter(largeMut, TEXTURE_FILTER_BILINEAR);
    SetFilter(smallMut, TEXTURE_FILTER_BILINEAR);

    DrawSample(y, "body",
               bodyMut, bodySz,
               "The quick brown fox jumps over the lazy dog.", docColor, TEXTURE_FILTER_BILINEAR);

    DrawSample(y, "heading",
               headingMut, headingSz,
               "The Quick Brown Fox Jumps", docColor, TEXTURE_FILTER_BILINEAR);

    DrawSample(y, "large",
               largeMut, largeSz,
               "The Gospel According to John", docColor, TEXTURE_FILTER_BILINEAR);

    DrawSample(y, "small",
               smallMut, smallSz,
               "1 2 3 4 5 6 7 8 9 10 11 12 13 14 15", docColor, TEXTURE_FILTER_BILINEAR);

    // ─── Bold comparison (bold font, POINT) ───
    DrawTextAt(bodyMut, "--- BOLD (POINT) ---", 10.0f, y + scrollY_, bodySz * 0.5f, DARKGRAY);
    y += bodySz * 0.5f + 12.0f;
    Font boldMut = boldFont_;
    SetFilter(boldMut, TEXTURE_FILTER_POINT);
    DrawSample(y, "body bold",
               boldMut, boldSz,
               "The quick brown fox jumps over the lazy dog.", docColor, TEXTURE_FILTER_POINT);

    DrawSample(y, "body bold",
               boldMut, boldSz,
               "The Quick Brown Fox Jumps In Bold", docColor, TEXTURE_FILTER_POINT);

    // Restore — body/large/small stay POINT, heading stays BILINEAR
    SetFilter(bodyMut, TEXTURE_FILTER_POINT);
    SetFilter(headingMut, TEXTURE_FILTER_BILINEAR);
    SetFilter(largeMut, TEXTURE_FILTER_POINT);
    SetFilter(smallMut, TEXTURE_FILTER_POINT);

    EndScissorMode();

    // ─── Non-scrolling header & footer ───
    char header[128];
    std::snprintf(header, sizeof(header),
                  "Font Diagnostic  |  dpiScale=%.2f  window=%dx%d  baseFontSize=%.0f  [SCROLL]",
                  dpiScale_, GetScreenWidth(), GetScreenHeight(), config::FONT_SIZE);
    DrawTextAt(GetFontDefault(), header, 10.0f, 8.0f, 14.0f, DARKGRAY);

    char footer[128];
    std::snprintf(footer, sizeof(footer),
                  "Body atlas=%dpx  Heading=%dpx  Large=%dpx  Small=%dpx  |  ESC to close",
                  bodyFont_.baseSize, headingFont_.baseSize,
                  largeFont_.baseSize, smallFont_.baseSize);
    DrawTextAt(GetFontDefault(), footer, 10.0f, screenH - 20.0f, 12.0f, GRAY);
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
