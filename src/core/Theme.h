#ifndef THEME_H
#define THEME_H

#include <raylib.h>

namespace theme {

// ── Document Text Colors ─────────────────────────────────────────────
constexpr Color DOC_BODY = BLACK;
constexpr Color DOC_HEADING = DARKGRAY;
constexpr Color DOC_BOOK_TITLE = BLACK;
constexpr Color DOC_CHAPTER_LABEL = {80, 80, 80, 255};
constexpr Color DOC_POETRY = DARKGRAY;

// ── UI Text Colors ───────────────────────────────────────────────────
constexpr Color UI_TEXT = DARKGRAY;
constexpr Color UI_TITLE = BLACK;
constexpr Color UI_INPUT_TEXT = BLACK;
constexpr Color UI_BUTTON_TEXT = BLACK;
constexpr Color UI_BUTTON_TEXT_DISABLED = GRAY;
constexpr Color UI_DELETE = RED;
constexpr Color UI_ERROR = RED;

// ── Panel & Window ───────────────────────────────────────────────────
constexpr Color PANEL_BG = WHITE;
constexpr Color PANEL_BORDER = DARKGRAY;
constexpr Color WINDOW_BG = RAYWHITE;
constexpr Color OVERLAY_BG = {0, 0, 0, 120};

// ── Interactive Elements ─────────────────────────────────────────────
constexpr Color BUTTON_BG = LIGHTGRAY;
constexpr Color BUTTON_BORDER = GRAY;
constexpr Color BUTTON_BORDER_DISABLED = LIGHTGRAY;
constexpr Color SELECTED_BG = SKYBLUE;
constexpr Color SWITCH_ON = SKYBLUE;
constexpr Color SWITCH_OFF = LIGHTGRAY;
constexpr Color INPUT_BG = LIGHTGRAY;
constexpr Color INPUT_BORDER = GRAY;
constexpr Color INPUT_BORDER_ERROR = RED;

// ── Scrollbar ────────────────────────────────────────────────────────
constexpr Color SCROLLBAR_THUMB = GRAY;

// ── Splash ───────────────────────────────────────────────────────────
constexpr Color SPLASH_TITLE = DARKGRAY;
constexpr Color SPLASH_SUBTITLE = LIGHTGRAY;

// ── Font Scales (× fontSize) ─────────────────────────────────────────
constexpr float FONT_HEADING = 1.3f;
constexpr float FONT_LARGE_HEADING = 1.6f;

// ── Font Scales (× headingSize) ──────────────────────────────────────
constexpr float FONT_LABEL = 0.8f;
constexpr float FONT_DETAIL = 0.7f;

} // namespace theme

#endif
