#include "SettingsScreen.h"
#include "NavigationStack.h"
#include "components.h"
#include "core/Theme.h"
#include "core/Config.h"
#include "core/Locale.h"
#include "event/EventBus.h"
#include "event/Events.h"
#include "persistence/PersistenceManager.h"
#include <algorithm>

namespace theword::ui {

using namespace theword::core;

SettingsScreen::SettingsScreen(NavigationStack& navStack,
                               theword::event::EventBus& eventBus,
                               theword::persistence::PersistenceManager& persistence,
                               const theword::core::UIScale& uiScale,
                               float& currentFontSize, int& currentBibleId, bool& immersiveMode,
                               const theword::core::ThemeManager& themeManager)
    : navStack_(navStack), eventBus_(eventBus),
      persistence_(persistence),
      uiScale_(uiScale), currentFontSize_(currentFontSize), currentBibleId_(currentBibleId), immersiveMode_(immersiveMode),
      themeManager_(themeManager),
      tapDetector_(uiScale_.dp(10)) {}

void SettingsScreen::Draw(theword::renderer::DrawContext& ctx) {
    const auto& palette = ctx.themeManager.Current();
    const Font& font = ctx.fonts.Get(theword::text::FontKind::Heading);
    float fontSize = ctx.fonts.HeadingSize();

    float headerH = uiScale_.dp(48);
    DrawHeaderBar(ctx, font, fontSize, Locale::Get("Settings"), true);

    float labelSize = fontSize * 0.7f;
    float controlSize = fontSize * 0.65f;

    float labelX = uiScale_.dp(16);
    float btnW = uiScale_.dp(36);
    float btnH = uiScale_.dp(30);
    float decX = uiScale_.dp(140);
    float valX = uiScale_.dp(190);
    float incX = uiScale_.dp(240);

    // Font row
    float rowY = headerH + uiScale_.dp(40);
    float rowGap = uiScale_.dp(48);

    DrawTextEx(font, Locale::Get("Font:"), {labelX, rowY}, labelSize, 1, palette.uiText);

    bool atMin = currentFontSize_ <= config::FONT_SIZE_MIN;
    bool atMax = currentFontSize_ >= config::FONT_SIZE_MAX;

    DrawButton(ctx, {decX, rowY, btnW, btnH}, "-", font, controlSize,
               !atMin, palette.uiButtonText, palette.buttonBg);

    std::string sizeStr = std::to_string(static_cast<int>(currentFontSize_));
    Vector2 sz = MeasureTextEx(font, sizeStr.c_str(), labelSize, 1);
    DrawTextEx(font, sizeStr.c_str(), {valX - sz.x / 2.0f, rowY + uiScale_.dp(4)},
               labelSize, 1, palette.uiText);

    DrawButton(ctx, {incX, rowY, btnW, btnH}, "+", font, controlSize,
               !atMax, palette.uiButtonText, palette.buttonBg);

    // Version row
    rowY += rowGap;
    DrawTextEx(font, Locale::Get("Version:"), {labelX, rowY}, labelSize, 1, palette.uiText);

    float verBtnW = uiScale_.dp(52);
    float verBtnH = uiScale_.dp(30);
    float verBtnGap = uiScale_.dp(4);
    float verStartX = uiScale_.dp(140);

    for (int i = 0; i < config::BIBLE_VERSION_COUNT; ++i) {
        float vx = verStartX + i * (verBtnW + verBtnGap);
        bool active = (currentBibleId_ == config::BIBLE_VERSIONS[i].id);
        DrawButton(ctx, {vx, rowY, verBtnW, verBtnH}, config::BIBLE_VERSIONS[i].label,
                   font, controlSize, true,
                   palette.uiText,
                   active ? palette.switchOn : palette.switchOff);
    }

    // Theme row
    rowY += rowGap;
    DrawTextEx(font, Locale::Get("Theme:"), {labelX, rowY}, labelSize, 1, palette.uiText);

    float thBtnW = uiScale_.dp(80);
    float thBtnH = uiScale_.dp(30);
    float thLightX = uiScale_.dp(140);
    float thDarkX = uiScale_.dp(230);

    bool isDark = themeManager_.IsDarkMode();
    DrawButton(ctx, {thLightX, rowY, thBtnW, thBtnH}, "Light", font, controlSize,
               true, palette.uiText, isDark ? palette.switchOff : palette.switchOn);
    DrawButton(ctx, {thDarkX, rowY, thBtnW, thBtnH}, "Dark", font, controlSize,
               true, palette.uiText, isDark ? palette.switchOn : palette.switchOff);

    // Immersive mode row
    rowY += rowGap;
    DrawTextEx(font, "Modo Limpo:", {labelX, rowY}, labelSize, 1, palette.uiText);

    float imBtnW = uiScale_.dp(80);
    float imBtnH = uiScale_.dp(30);
    float imOnX = uiScale_.dp(140);
    float imOffX = uiScale_.dp(230);

    DrawButton(ctx, {imOnX, rowY, imBtnW, imBtnH}, "ON", font, controlSize,
               true, palette.uiText, immersiveMode_ ? palette.switchOn : palette.switchOff);
    DrawButton(ctx, {imOffX, rowY, imBtnW, imBtnH}, "OFF", font, controlSize,
               true, palette.uiText, immersiveMode_ ? palette.switchOff : palette.switchOn);

    float hitW = uiScale_.dp(56);
    bool overControls = ctx.input.mouseY >= headerH + uiScale_.dp(40)
        || (ctx.input.mouseY < headerH && ctx.input.mouseX < hitW);
    ctx.SetCursor(overControls ? MOUSE_CURSOR_POINTING_HAND : MOUSE_CURSOR_DEFAULT);
}

bool SettingsScreen::HandleInput(const theword::renderer::DrawContext& ctx, float /*deltaTime*/) {
    if (ctx.input.KeyPressed(key::ESCAPE)) {
        navStack_.Pop();
        return true;
    }

    if (ctx.input.leftPressed)
        tapDetector_.OnPress(ctx.input.mouseX, ctx.input.mouseY);

    if (ctx.input.leftReleased) {
        float tapX, tapY;
        auto tr = tapDetector_.OnRelease(ctx.input.mouseX, ctx.input.mouseY, tapX, tapY);
        if (tr == TapDetector::Result::Drag) { return false; }
        if (tr == TapDetector::Result::Tap) {

        float headerH = uiScale_.dp(48);
        float backW = uiScale_.dp(56);

        if (tapY < headerH && tapX < backW) {
            navStack_.Pop();
            return true;
        }

        float btnW = uiScale_.dp(36);
        float btnH = uiScale_.dp(30);
        float decX = uiScale_.dp(140);
        float incX = uiScale_.dp(240);

        float rowY = headerH + uiScale_.dp(40);
        float rowGap = uiScale_.dp(48);

        // Font decrease
        Rectangle decRect = {decX, rowY, btnW, btnH};
        if (PointInRect(tapX, tapY, decRect)) {
            ChangeFontSize(-config::FONT_SIZE_STEP);
            return true;
        }

        // Font increase
        Rectangle incRect = {incX, rowY, btnW, btnH};
        if (PointInRect(tapX, tapY, incRect)) {
            ChangeFontSize(config::FONT_SIZE_STEP);
            return true;
        }

        // Version row
        rowY += rowGap;
        float verBtnW = uiScale_.dp(52);
        float verBtnH = uiScale_.dp(30);
        float verBtnGap = uiScale_.dp(4);
        float verStartX = uiScale_.dp(140);

        for (int i = 0; i < config::BIBLE_VERSION_COUNT; ++i) {
            float vx = verStartX + i * (verBtnW + verBtnGap);
            Rectangle verRect = {vx, rowY, verBtnW, verBtnH};
            if (PointInRect(tapX, tapY, verRect)) {
                int newId = config::BIBLE_VERSIONS[i].id;
                if (newId != currentBibleId_) {
                    currentBibleId_ = newId;
                    eventBus_.Emit(theword::event::BibleVersionSwitchEvent{newId});
                }
                return true;
            }
        }

        // Theme row
        rowY += rowGap;
        float thBtnW = uiScale_.dp(80);
        float thBtnH = uiScale_.dp(30);
        float thLightX = uiScale_.dp(140);
        float thDarkX = uiScale_.dp(230);
        Rectangle thLightRect = {thLightX, rowY, thBtnW, thBtnH};
        Rectangle thDarkRect = {thDarkX, rowY, thBtnW, thBtnH};
        if (PointInRect(tapX, tapY, thLightRect)) {
            if (themeManager_.IsDarkMode()) {
                eventBus_.Emit(theword::event::ThemeToggleEvent{});
            }
            return true;
        }
        if (PointInRect(tapX, tapY, thDarkRect)) {
            if (!themeManager_.IsDarkMode()) {
                eventBus_.Emit(theword::event::ThemeToggleEvent{});
            }
            return true;
        }

        // Immersive mode row
        rowY += rowGap;
        float imBtnW = uiScale_.dp(80);
        float imBtnH = uiScale_.dp(30);
        float imOnX = uiScale_.dp(140);
        float imOffX = uiScale_.dp(230);
        Rectangle imOnRect = {imOnX, rowY, imBtnW, imBtnH};
        Rectangle imOffRect = {imOffX, rowY, imBtnW, imBtnH};
        if (PointInRect(tapX, tapY, imOnRect)) {
            immersiveMode_ = true;
            persistence_.SetPreference("immersive_mode", "1");
            return true;
        }
        if (PointInRect(tapX, tapY, imOffRect)) {
            immersiveMode_ = false;
            persistence_.SetPreference("immersive_mode", "0");
            return true;
        }
    }
    }

    return false;
}

void SettingsScreen::ChangeFontSize(float delta) {
    float newSize = currentFontSize_ + delta;
    newSize = std::max(config::FONT_SIZE_MIN, std::min(config::FONT_SIZE_MAX, newSize));
    if (newSize != currentFontSize_) {
        currentFontSize_ = newSize;
        persistence_.SetPreference("font_size", std::to_string(static_cast<int>(newSize)));
        eventBus_.Emit(theword::event::FontSizeEvent{newSize * uiScale_.dpiScale, 0.0f});
    }
}

} // namespace theword::ui
