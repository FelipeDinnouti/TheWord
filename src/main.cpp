#include "raylib.h"
#include "core/Config.h"
#include "core/EnvLoader.h"
#include "core/APIClient.h"
#include "data/BibleClient.h"
#include "text/LayoutEngine.h"
#include <cstdlib>
#include <iostream>
#include <string>

std::string stripHtmlTags(const std::string& html) {
    std::string result;
    bool inTag = false;
    for (char c : html) {
        if (c == '<') {
            inTag = true;
        } else if (c == '>') {
            inTag = false;
        } else if (!inTag) {
            result += c;
        }
    }
    std::string cleaned;
    for (char c : result) {
        if (c != '\n' && c != '\r' && c != '\t') {
            if (c != ' ' || (cleaned.empty() || cleaned.back() != ' ')) {
                cleaned += c;
            }
        }
    }
    return cleaned;
}

int main() {
    EnvLoader::load(config::ENV_FILE);
    std::string appKey = EnvLoader::get(config::YVP_APP_KEY);

    InitWindow(config::WINDOW_WIDTH, config::WINDOW_HEIGHT, "TheWord");
    SetTargetFPS(config::TARGET_FPS);

    // Font font = GetFontDefault();
    Font font = LoadFontEx("data/source_serif_4/SourceSerif4-Regular.ttf", 64, NULL, 0);

    LayoutEngine layoutEngine(config::WINDOW_WIDTH - 40.0f, font, config::FONT_SIZE, config::LINE_SPACING);

    std::string verseText;
    std::string verseReference = "John 3:16";

    if (!appKey.empty()) {
        BibleClient client(appKey);
        BiblePassage passage = client.getPassage(config::DEFAULT_BIBLE_ID, config::DEFAULT_VERSE, "text");

        if (!passage.content.empty()) {
            verseText = stripHtmlTags(passage.content);
            verseReference = passage.reference.empty() ? verseReference : passage.reference;
        } else {
            verseText = "(PLACEHOLDER) For God so loved the world that he gave his one and only Son, that whoever believes in him shall not perish but have eternal life.";
        }
    } else {
        verseText = "(PLACEHOLDER) For God so loved the world that he gave his one and only Son, that whoever believes in him shall not perish but have eternal life.";
    }

    ChapterLayout layout = layoutEngine.layoutChapter(config::DEFAULT_VERSE, verseText);

    float scrollY = 0.0f;
    float scrollVelocity = 0.0f;
    const float scrollSensitivity = 30.0f;
    const float friction = 0.92f;
    const float minVelocity = 0.1f;

    while (!WindowShouldClose()) {
        float wheel = GetMouseWheelMove();
        if (wheel != 0) {
            scrollVelocity -= wheel * scrollSensitivity;
        }

        if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) {
            scrollVelocity += scrollSensitivity * 0.16f;
        }
        if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) {
            scrollVelocity -= scrollSensitivity * 0.16f;
        }

        scrollVelocity *= friction;
        if (std::abs(scrollVelocity) < minVelocity) {
            scrollVelocity = 0.0f;
        }

        float maxScroll = layout.totalHeight - GetScreenHeight();
        if (maxScroll < 0) maxScroll = 0;

        scrollY += scrollVelocity;
        if (scrollY < 0) {
            scrollY = 0;
            scrollVelocity = 0;
        }
        if (scrollY > maxScroll) {
            scrollY = maxScroll;
            scrollVelocity = 0;
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        if (!verseReference.empty()) {
            DrawText(verseReference.c_str(), 20, 20, 18, DARKGRAY);
        }

        float contentTop = 60.0f;
        float viewHeight = (float)GetScreenHeight() - contentTop;

        for (size_t lineIdx = 0; lineIdx < layout.lines.size(); ++lineIdx) {
            const Line& line = layout.lines[lineIdx];

            float screenY = line.y - scrollY + contentTop;
            if (screenY < -line.height || screenY > GetScreenHeight()) {
                continue;
            }

            for (const auto& span : line.spans) {
                float textX = span.x;
                float textY = screenY + (line.height - span.height) / 2;

                DrawTextEx(font, span.text.c_str(), {textX, textY}, config::FONT_SIZE, 1, BLACK);
            }
        }

        float scrollBarHeight = viewHeight * (viewHeight / layout.totalHeight);
        if (scrollBarHeight < 20) scrollBarHeight = 20;

        float scrollBarY = contentTop + (scrollY / layout.totalHeight) * (viewHeight - scrollBarHeight);

        DrawRectangle(GetScreenWidth() - 6, (int)scrollBarY, 4, (int)scrollBarHeight, LIGHTGRAY);

        DrawFPS(10, GetScreenHeight() - 30);

        if (appKey.empty()) {
            const char* warning = "Set YVP_APP_KEY env var for live API";
            DrawText(warning, 10, GetScreenHeight() - 30, 14, LIGHTGRAY);
        }

        EndDrawing();
    }

    UnloadFont(font);

    CloseWindow();
    return 0;
}
