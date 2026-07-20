#ifndef THEME_H
#define THEME_H

#include <raylib.h>

namespace theword::core { namespace theme {

constexpr float PANEL_ROUNDING = 6.0f;
constexpr float HOVER_DARKEN = 0.85f;
constexpr float PRESS_DARKEN = 0.70f;

inline Color Darken(Color c, float factor) {
    return Color{
        static_cast<unsigned char>(c.r * factor),
        static_cast<unsigned char>(c.g * factor),
        static_cast<unsigned char>(c.b * factor),
        c.a
    };
}

constexpr float FONT_HEADING = 1.3f;
constexpr float FONT_LARGE_HEADING = 1.6f;

constexpr float FONT_LABEL = 0.8f;
constexpr float FONT_DETAIL = 0.7f;
constexpr float FONT_VERSE_NUMBER = 0.65f;

} } // namespace theword::core::theme

#endif
