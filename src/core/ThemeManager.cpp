#include "ThemeManager.h"

namespace theword::core {

ThemePalette ThemePalette::Light() {
    return {
        BLACK,              // docBody
        DARKGRAY,           // docHeading
        BLACK,              // docBookTitle
        {80, 80, 80, 255},  // docChapterLabel
        DARKGRAY,           // docPoetry
        {160, 160, 160, 255}, // docVerseNumber
        {100, 100, 180, 255}, // docFootnoteCaller
        {255, 250, 240, 255}, // docFootnotePopupBg
        {200, 180, 150, 255}, // docFootnotePopupBorder
        {40, 35, 30, 255},   // docFootnotePopupText

        DARKGRAY,           // uiText
        BLACK,              // uiTitle
        BLACK,              // uiInputText
        BLACK,              // uiButtonText
        GRAY,               // uiButtonTextDisabled
        RED,                // uiDelete
        RED,                // uiError

        WHITE,              // panelBg
        DARKGRAY,           // panelBorder
        RAYWHITE,           // windowBg
        {0, 0, 0, 120},     // overlayBg

        LIGHTGRAY,          // buttonBg
        GRAY,               // buttonBorder
        LIGHTGRAY,          // buttonBorderDisabled
        SKYBLUE,            // selectedBg
        SKYBLUE,            // switchOn
        LIGHTGRAY,          // switchOff
        LIGHTGRAY,          // inputBg
        GRAY,               // inputBorder
        RED,                // inputBorderError

        {14, 165, 233, 255}, // accentTeal
        GRAY,               // scrollbarThumb
        DARKGRAY,           // splashTitle
        LIGHTGRAY           // splashSubtitle
    };
}

ThemePalette ThemePalette::Dark() {
    return {
        {230, 230, 230, 255}, // docBody
        {200, 200, 200, 255}, // docHeading
        {240, 240, 240, 255}, // docBookTitle
        {160, 160, 160, 255}, // docChapterLabel
        {200, 200, 200, 255}, // docPoetry
        {140, 140, 140, 255}, // docVerseNumber
        {130, 140, 200, 255}, // docFootnoteCaller
        {50, 45, 40, 255},    // docFootnotePopupBg
        {120, 110, 90, 255},  // docFootnotePopupBorder
        {220, 215, 210, 255}, // docFootnotePopupText

        {210, 210, 210, 255}, // uiText
        {240, 240, 240, 255}, // uiTitle
        {220, 220, 220, 255}, // uiInputText
        {220, 220, 220, 255}, // uiButtonText
        {100, 100, 100, 255}, // uiButtonTextDisabled
        {255, 80, 80, 255},   // uiDelete
        {255, 80, 80, 255},   // uiError

        {45, 45, 45, 255},    // panelBg
        {100, 100, 100, 255}, // panelBorder
        {30, 30, 30, 255},    // windowBg
        {0, 0, 0, 180},       // overlayBg

        {60, 60, 60, 255},    // buttonBg
        {120, 120, 120, 255}, // buttonBorder
        {60, 60, 60, 255},    // buttonBorderDisabled
        {30, 60, 100, 255},   // selectedBg
        {50, 100, 160, 255},  // switchOn
        {60, 60, 60, 255},    // switchOff
        {60, 60, 60, 255},    // inputBg
        {120, 120, 120, 255}, // inputBorder
        {255, 80, 80, 255},   // inputBorderError

        {14, 165, 233, 255},  // accentTeal
        {100, 100, 100, 255}, // scrollbarThumb
        {200, 200, 200, 255}, // splashTitle
        {130, 130, 130, 255}  // splashSubtitle
    };
}

ThemeManager::ThemeManager()
    : light_(ThemePalette::Light()), dark_(ThemePalette::Dark()), active_(&light_) {}

void ThemeManager::SetDarkMode(bool dark) {
    active_ = dark ? &dark_ : &light_;
}

bool ThemeManager::IsDarkMode() const {
    return active_ == &dark_;
}

void ThemeManager::Toggle() {
    active_ = (active_ == &light_) ? &dark_ : &light_;
}

const ThemePalette& ThemeManager::Current() const {
    return *active_;
}

} // namespace theword::core
