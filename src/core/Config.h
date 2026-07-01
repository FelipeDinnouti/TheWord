#ifndef CONFIG_H
#define CONFIG_H

namespace theword::core { namespace config {

constexpr int WINDOW_WIDTH = 450;
constexpr int WINDOW_HEIGHT = 800;
constexpr int TARGET_FPS = 60;
constexpr int IDLE_DRAIN_FRAMES = 30;
constexpr int IDLE_DRAIN_INTERVAL = 12;
constexpr float CONTENT_PADDING = 40.0f;
constexpr float TOP_BAR_HEIGHT = 60.0f;

constexpr float FONT_SIZE = 24.0f;
constexpr float FONT_SIZE_MIN = 12.0f;
constexpr float FONT_SIZE_MAX = 36.0f;
constexpr float FONT_SIZE_STEP = 2.0f;
constexpr float FONT_HEADING_SIZE = FONT_SIZE * 1.3f;
constexpr float LINE_SPACING = 1.2f;

#ifdef __ANDROID__
// AAssetManager paths are relative to APK assets root — no "assets/" prefix
constexpr const char* USFM_DIR = "usfm";
constexpr const char* FONT_REGULAR = "fonts/source_serif_4/SourceSerif4-Regular.ttf";
constexpr const char* FONT_BOLD = "fonts/source_serif_4/SourceSerif4-Bold.ttf";

// Android: raylib stores raw AKEYCODE values, not raylib KEY_* constants
constexpr int KEY_ESCAPE_ANDROID = 4;      // AKEYCODE_BACK
constexpr int KEY_G_ANDROID = 35;          // AKEYCODE_G
constexpr int KEY_S_ANDROID = 47;          // AKEYCODE_S
constexpr int KEY_A_ANDROID = 29;          // AKEYCODE_A
constexpr int KEY_UP_ANDROID = 19;         // AKEYCODE_DPAD_UP
constexpr int KEY_DOWN_ANDROID = 20;       // AKEYCODE_DPAD_DOWN
constexpr int KEY_ENTER_ANDROID = 66;      // AKEYCODE_ENTER
constexpr int KEY_TAB_ANDROID = 61;        // AKEYCODE_TAB
constexpr int KEY_BACKSPACE_ANDROID = 67;  // AKEYCODE_DEL
constexpr int KEY_LEFT_ANDROID = 21;       // AKEYCODE_DPAD_LEFT
constexpr int KEY_RIGHT_ANDROID = 22;      // AKEYCODE_DPAD_RIGHT
#else
constexpr const char* USFM_DIR = "assets/usfm";
constexpr const char* FONT_REGULAR = "assets/fonts/source_serif_4/SourceSerif4-Regular.ttf";
constexpr const char* FONT_BOLD = "assets/fonts/source_serif_4/SourceSerif4-Bold.ttf";
#endif

constexpr const char* ENV_FILE = ".env";
constexpr const char* YVP_APP_KEY = "YVP_APP_KEY";

constexpr const char* DB_DIR = ".theword";
constexpr const char* DB_FILE = "highlights.db";

} } // namespace theword::core::config

// On Android, raylib stores raw AKEYCODE values instead of raylib KEY_* constants.
// Use key::ESCAPE, key::G, etc. for platform-agnostic key codes.
namespace theword::core { namespace key {
#ifdef __ANDROID__
    constexpr int ESCAPE = config::KEY_ESCAPE_ANDROID;
    constexpr int G = config::KEY_G_ANDROID;
    constexpr int S = config::KEY_S_ANDROID;
    constexpr int A = config::KEY_A_ANDROID;
    constexpr int UP = config::KEY_UP_ANDROID;
    constexpr int DOWN = config::KEY_DOWN_ANDROID;
    constexpr int ENTER = config::KEY_ENTER_ANDROID;
    constexpr int TAB = config::KEY_TAB_ANDROID;
    constexpr int BACKSPACE = config::KEY_BACKSPACE_ANDROID;
    constexpr int LEFT = config::KEY_LEFT_ANDROID;
    constexpr int RIGHT = config::KEY_RIGHT_ANDROID;
#else
    constexpr int ESCAPE = KEY_ESCAPE;
    constexpr int G = KEY_G;
    constexpr int S = KEY_S;
    constexpr int A = KEY_A;
    constexpr int UP = KEY_UP;
    constexpr int DOWN = KEY_DOWN;
    constexpr int LEFT = KEY_LEFT;
    constexpr int RIGHT = KEY_RIGHT;
    constexpr int ENTER = KEY_ENTER;
    constexpr int TAB = KEY_TAB;
    constexpr int BACKSPACE = KEY_BACKSPACE;
#endif
} } // namespace theword::core::key

#endif // CONFIG_H
