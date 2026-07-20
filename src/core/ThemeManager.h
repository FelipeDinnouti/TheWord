#ifndef THEME_MANAGER_H
#define THEME_MANAGER_H

#include <raylib.h>

namespace theword::core {

struct ThemePalette {
    Color docBody;
    Color docHeading;
    Color docBookTitle;
    Color docChapterLabel;
    Color docPoetry;
    Color docVerseNumber;
    Color docFootnoteCaller;
    Color docFootnotePopupBg;
    Color docFootnotePopupBorder;
    Color docFootnotePopupText;

    Color uiText;
    Color uiTitle;
    Color uiInputText;
    Color uiButtonText;
    Color uiButtonTextDisabled;
    Color uiDelete;
    Color uiError;

    Color panelBg;
    Color panelBorder;
    Color windowBg;
    Color overlayBg;

    Color buttonBg;
    Color buttonBorder;
    Color buttonBorderDisabled;
    Color selectedBg;
    Color switchOn;
    Color switchOff;
    Color inputBg;
    Color inputBorder;
    Color inputBorderError;

    Color accentTeal;
    Color scrollbarThumb;
    Color splashTitle;
    Color splashSubtitle;

    static ThemePalette Light();
    static ThemePalette Dark();
};

class ThemeManager {
public:
    ThemeManager();

    void SetDarkMode(bool dark);
    bool IsDarkMode() const;
    void Toggle();
    const ThemePalette& Current() const;

private:
    ThemePalette light_;
    ThemePalette dark_;
    const ThemePalette* active_;
};

} // namespace theword::core

#endif
