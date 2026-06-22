#ifndef CONFIG_H
#define CONFIG_H

#include <string>

namespace config {

constexpr int WINDOW_WIDTH = 450;
constexpr int WINDOW_HEIGHT = 800;
constexpr int TARGET_FPS = 60;

constexpr float FONT_SIZE = 24.0f;
constexpr float FONT_HEADING_SIZE = FONT_SIZE * 1.3f;
constexpr float LINE_SPACING = 1.2f;

constexpr const char* USFM_DIR = "assets/usfm";
constexpr const char* FONT_REGULAR = "assets/fonts/source_serif_4/SourceSerif4-Regular.ttf";
constexpr const char* FONT_BOLD = "assets/fonts/source_serif_4/SourceSerif4-Bold.ttf";

constexpr const char* ENV_FILE = ".env";
constexpr const char* YVP_APP_KEY = "YVP_APP_KEY";

constexpr const char* DB_DIR = ".theword";
constexpr const char* DB_FILE = "highlights.db";

} // namespace config

#endif // CONFIG_H
