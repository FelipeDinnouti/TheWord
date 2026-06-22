#include "raylib.h"
#include "core/Config.h"
#include "core/BibleBooks.h"
#include "core/APIClient.h"
#include "core/EnvLoader.h"
#include "data/USFMParser.h"
#include "data/BibleClient.h"
#include "data/CompositeProvider.h"
#include "text/LayoutEngine.h"
#include "document/DocumentManager.h"
#include "renderer/Renderer.h"
#include "highlight/Highlighter.h"
#include "persistence/PersistenceManager.h"
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

int main() {
    InitWindow(config::WINDOW_WIDTH, config::WINDOW_HEIGHT, "TheWord");
    SetTargetFPS(config::TARGET_FPS);

    std::vector<int> codepoints;
    for (int i = 32; i < 127; i++) codepoints.push_back(i);
    for (int i = 160; i < 256; i++) codepoints.push_back(i);
    codepoints.push_back(0x2013);  // EN DASH
    codepoints.push_back(0x2018);  // LEFT SINGLE QUOTATION MARK
    codepoints.push_back(0x2019);  // RIGHT SINGLE QUOTATION MARK
    codepoints.push_back(0x201C);  // LEFT DOUBLE QUOTATION MARK
    codepoints.push_back(0x201D);  // RIGHT DOUBLE QUOTATION MARK
    codepoints.push_back(0x2026);  // HORIZONTAL ELLIPSIS

    Font bodyFont = LoadFontEx(config::FONT_REGULAR, (int)config::FONT_SIZE,
                               codepoints.data(), (int)codepoints.size());
    SetTextureFilter(bodyFont.texture, TEXTURE_FILTER_POINT);

    Font headingFont = LoadFontEx(config::FONT_REGULAR, (int)config::FONT_HEADING_SIZE,
                                  codepoints.data(), (int)codepoints.size());
    SetTextureFilter(headingFont.texture, TEXTURE_FILTER_POINT);

    float contentTop = 60.0f;
    float contentWidth = config::WINDOW_WIDTH - 40.0f;
    float viewportHeight = config::WINDOW_HEIGHT - contentTop;

    LayoutEngine layoutEngine(contentWidth, bodyFont, config::FONT_SIZE, config::LINE_SPACING);
    Renderer renderer(bodyFont, headingFont, contentTop, config::FONT_SIZE);
    USFMParser usfmParser(config::USFM_DIR);

    EnvLoader::load(config::ENV_FILE);
    std::string apiKey = EnvLoader::get(config::YVP_APP_KEY);

    std::unique_ptr<APIClient> apiClient;
    std::unique_ptr<BibleClient> bibleClient;
    std::unique_ptr<CompositeProvider> compositeProvider;
    ChapterProvider* activeProvider = &usfmParser;

    if (!apiKey.empty()) {
        apiClient = std::make_unique<APIClient>();
        apiClient->setAppKey(apiKey);
        bibleClient = std::make_unique<BibleClient>(*apiClient, 3034);
        compositeProvider = std::make_unique<CompositeProvider>(*bibleClient, usfmParser);
        activeProvider = compositeProvider.get();
    }

    std::string home = getenv("HOME") ? getenv("HOME") : ".";
    std::string dbPath = home + "/" + config::DB_DIR + "/" + config::DB_FILE;
    DocumentManager documentManager(layoutEngine, viewportHeight, *activeProvider, contentTop);
    PersistenceManager storage(dbPath);
    Highlighter highlighter(storage);

    documentManager.loadInitialChapter("GEN.1");

    float scrollVelocity = 0.0f;
    const float scrollSensitivity = 30.0f;
    const float friction = 0.92f;
    const float minVelocity = 0.1f;

    double lastTime = GetTime();

    while (!WindowShouldClose()) {
        double currentTime = GetTime();
        float deltaTime = (float)(currentTime - lastTime);
        lastTime = currentTime;

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

        documentManager.scrollBy(scrollVelocity);
        documentManager.update(deltaTime);

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Vector2 m = GetMousePosition();
            int wordId = documentManager.hitTestWord(m.x, m.y, documentManager.getScrollY());
            if (wordId >= 0) highlighter.startSelection(wordId);
        }
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            Vector2 m = GetMousePosition();
            int wordId = documentManager.hitTestWord(m.x, m.y, documentManager.getScrollY());
            if (wordId >= 0) highlighter.updateSelection(wordId);
        }
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            highlighter.endSelection();
        }

        if (IsWindowResized()) {
            float newContentWidth = GetScreenWidth() - 40.0f;
            layoutEngine.setMaxWidth(newContentWidth);
            layoutEngine.invalidateCache();
            documentManager.invalidateLayouts();
            documentManager.setViewportHeight(GetScreenHeight() - contentTop);
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        float scrollY = documentManager.getScrollY();
        float totalHeight = documentManager.getTotalHeight();
        float viewHeight = documentManager.getViewportHeight();

        std::vector<std::pair<Span, float>> docSpans;
        documentManager.getVisibleSpans(docSpans);

        std::vector<HighlightRect> hlRects;
        for (const auto& [span, docY] : docSpans) {
            if (span.startWord >= 0 && highlighter.isWordHighlighted(span.startWord)) {
                float screenY = docY - scrollY + contentTop;
                hlRects.push_back({span.x, screenY, span.width, span.height,
                                   highlighter.getHighlightForWord(span.startWord)});
            }
        }

        renderer.drawFrame(scrollY, totalHeight, viewHeight, docSpans,
                           documentManager.getChapterTitle(), hlRects);
        renderer.drawFpsCounter(10, GetScreenHeight() - 30);

        EndDrawing();
    }

    UnloadFont(bodyFont);
    UnloadFont(headingFont);
    CloseWindow();
    return 0;
}
