#ifndef CONFIG_H
#define CONFIG_H

namespace config {

constexpr int WINDOW_WIDTH = 450;
constexpr int WINDOW_HEIGHT = 800;
constexpr int TARGET_FPS = 60;

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
#else
constexpr const char* USFM_DIR = "assets/usfm";
constexpr const char* FONT_REGULAR = "assets/fonts/source_serif_4/SourceSerif4-Regular.ttf";
constexpr const char* FONT_BOLD = "assets/fonts/source_serif_4/SourceSerif4-Bold.ttf";
#endif

constexpr const char* ENV_FILE = ".env";
constexpr const char* YVP_APP_KEY = "YVP_APP_KEY";

constexpr const char* DB_DIR = ".theword";
constexpr const char* DB_FILE = "highlights.db";

} // namespace config

#endif // CONFIG_H
