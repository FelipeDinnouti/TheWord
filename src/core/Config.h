#ifndef CONFIG_H
#define CONFIG_H

#include <string>

namespace config {

constexpr int WINDOW_WIDTH = 450;
constexpr int WINDOW_HEIGHT = 800;
constexpr int TARGET_FPS = 60;

constexpr float FONT_SIZE = 20.0f;
constexpr float LINE_SPACING = 1.2f;

constexpr const char* API_BASE_URL = "https://api.youversion.com/v1";
constexpr int DEFAULT_BIBLE_ID = 3034;
constexpr const char* DEFAULT_VERSE = "JHN.3.16";

constexpr const char* ENV_FILE = ".env";
constexpr const char* YVP_APP_KEY = "YVP_APP_KEY";

} // namespace config

#endif // CONFIG_H
