#include "raylib.h"
#include "core/Config.h"
#include "core/FontHelper.h"
#include "core/BibleBooks.h"
#include "core/APIClient.h"
#include "core/EnvLoader.h"
#include "data/USFMParser.h"
#include "data/BibleClient.h"
#include "data/CompositeProvider.h"
#include "text/LayoutEngine.h"
#include "document/DocumentManager.h"
#include "renderer/Renderer.h"
#include "renderer/UIManager.h"
#include "input/InputHandler.h"
#include "highlight/Highlighter.h"
#include "persistence/PersistenceManager.h"
#include <cstdlib>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>
#include <algorithm>

int main() {
    InitWindow(config::WINDOW_WIDTH, config::WINDOW_HEIGHT, "TheWord");
    SetTargetFPS(config::TARGET_FPS);

    std::vector<int> codepoints = LoadFontCodepoints(config::FONT_REGULAR);

    Font bodyFont = LoadFontEx(config::FONT_REGULAR, (int)config::FONT_SIZE,
                               codepoints.data(), (int)codepoints.size());
    SetTextureFilter(bodyFont.texture, TEXTURE_FILTER_POINT);

    Font headingFont = LoadFontEx(config::FONT_REGULAR, (int)config::FONT_HEADING_SIZE,
                                  codepoints.data(), (int)codepoints.size());
    SetTextureFilter(headingFont.texture, TEXTURE_FILTER_POINT);

    float headingSize = config::FONT_SIZE * 1.3f;
    float contentWidth = config::WINDOW_WIDTH - 40.0f;

    EnvLoader::load(config::ENV_FILE);
    std::string apiKey = EnvLoader::get(config::YVP_APP_KEY);

    USFMParser usfmParser(config::USFM_DIR);

    std::unique_ptr<APIClient> apiClient;
    std::unique_ptr<BibleClient> bibleClient;
    std::unique_ptr<CompositeProvider> compositeProvider;

    ChapterProvider* onlineProv = &usfmParser;
    ChapterProvider* offlineProv = &usfmParser;
    ChapterProvider* activeProv = &usfmParser;

    if (!apiKey.empty()) {
        apiClient = std::make_unique<APIClient>();
        apiClient->setAppKey(apiKey);
        bibleClient = std::make_unique<BibleClient>(*apiClient, 3034);
        compositeProvider = std::make_unique<CompositeProvider>(*bibleClient, usfmParser);
        onlineProv = bibleClient.get();
        offlineProv = &usfmParser;
        activeProv = compositeProvider.get();
    }

    std::string home = getenv("HOME") ? getenv("HOME") : ".";
    std::string dbPath = home + "/" + config::DB_DIR + "/" + config::DB_FILE;
    PersistenceManager storage(dbPath);
    Highlighter highlighter(storage);

    float fontSize = config::FONT_SIZE;
    std::string savedFontSize = storage.getPreference("font_size", "");
    if (!savedFontSize.empty()) {
        fontSize = std::max(12.0f, std::min(36.0f, (float)std::atoi(savedFontSize.c_str())));
    }

    LayoutEngine layoutEngine(contentWidth, bodyFont, fontSize, config::LINE_SPACING);
    Renderer renderer(bodyFont, headingFont, 60.0f, fontSize);

    float viewportHeight = config::WINDOW_HEIGHT - 60.0f;
    DocumentManager documentManager(layoutEngine, viewportHeight, *activeProv, 60.0f);

    bool versionOnline = false;
    if (compositeProvider) {
        std::string savedVersion = storage.getPreference("active_version", "online");
        if (savedVersion == "offline") {
            compositeProvider->setPrimary(*offlineProv);
            versionOnline = false;
        } else {
            compositeProvider->setPrimary(*onlineProv);
            versionOnline = true;
        }
    }

    highlighter.setProvider(versionOnline ? "BibleClient" : "USFMParser");

    UIManager uiManager(headingFont, headingSize, highlighter,
                        documentManager, layoutEngine, renderer, storage,
                        *onlineProv, *offlineProv, compositeProvider.get(),
                        fontSize, versionOnline);

    std::string savedColor = storage.getPreference("active_color", "");
    if (!savedColor.empty()) {
        highlighter.setActiveTypeId(std::atoi(savedColor.c_str()));
    }

    InputHandler inputHandler(documentManager, highlighter, layoutEngine, uiManager, uiManager.getContentTop());

    documentManager.loadInitialChapter("GEN.1");

    double lastTime = GetTime();

    while (!WindowShouldClose()) {
        double currentTime = GetTime();
        float deltaTime = (float)(currentTime - lastTime);
        lastTime = currentTime;

        inputHandler.handleInput(deltaTime);
        documentManager.update(deltaTime);

        BeginDrawing();
        ClearBackground(RAYWHITE);

        float scrollY = documentManager.getScrollY();
        float totalHeight = documentManager.getTotalHeight();
        float viewHeight = documentManager.getViewportHeight();

        uiManager.drawTopBar(documentManager.getChapterTitle());

        std::vector<std::pair<Span, float>> docSpans;
        documentManager.getVisibleSpans(docSpans);

        std::vector<HighlightRect> hlRects;
        for (const auto& [span, docY] : docSpans) {
            if (span.startWord >= 0 && highlighter.isWordHighlighted(span.startWord)) {
                float screenY = docY - scrollY + uiManager.getContentTop();
                hlRects.push_back({span.x, screenY, span.width, span.height,
                                   highlighter.getHighlightForWord(span.startWord)});
            }
        }

        renderer.drawFrame(scrollY, totalHeight, viewHeight, docSpans, hlRects);
        uiManager.drawContextMenu();
        uiManager.drawGoToDialog();
        uiManager.drawSettingsPanel();
#ifndef NDEBUG
        renderer.drawFpsCounter(10, GetScreenHeight() - 30);
#endif

        EndDrawing();
    }

    UnloadFont(bodyFont);
    UnloadFont(headingFont);
    CloseWindow();
    return 0;
}
