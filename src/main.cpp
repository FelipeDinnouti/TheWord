#include "raylib.h"
#include "core/Config.h"
#include "core/FontHelper.h"
#include "core/BibleBooks.h"
#include "core/IHttpClient.h"
#include "core/FileAssetProvider.h"
#include "core/Logger.h"
#ifdef __EMSCRIPTEN__
#include "core/EmscriptenClient.h"
#elif defined(__ANDROID__)
#include <android_native_app_glue.h>
#include <android/asset_manager.h>
#include "core/AndroidClient.h"
#include "core/AndroidAssetProvider.h"
extern "C" struct android_app* GetAndroidApp(void);
#else
#include "core/CurlHttpClient.h"
#endif
#include "core/EnvLoader.h"
#include "data/USFMParser.h"
#include "data/BibleClient.h"
#include "data/CompositeProvider.h"
#include "text/LayoutEngine.h"
#include "document/DocumentManager.h"
#include "renderer/Renderer.h"
#include "renderer/UIManager.h"
#include "input/InputHandler.h"
#include "core/Theme.h"
#include "highlight/Highlighter.h"
#include "persistence/PersistenceManager.h"
#include <cstdlib>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>
#include <algorithm>

int main() {
    Logger::Info("Starting TheWord");

#if defined(__ANDROID__)
    ANativeWindow* mainWin = GetAndroidApp()->window;
    int screenW = ANativeWindow_getWidth(mainWin);
    int screenH = ANativeWindow_getHeight(mainWin);
#else
    int screenW = config::WINDOW_WIDTH;
    int screenH = config::WINDOW_HEIGHT;
#endif
    float scale = (float)screenW / config::WINDOW_WIDTH;

    InitWindow(screenW, screenH, "TheWord");
    Logger::Info("Window initialized");
    SetTargetFPS(config::TARGET_FPS);

    {
        const char* title = "TheWord";
        const char* subtitle = "Loading...";
        float titleSize = 48.0f * scale;
        float subSize = 20.0f * scale;
        Vector2 titleDims = MeasureTextEx(GetFontDefault(), title, titleSize, 1);
        Vector2 subDims = MeasureTextEx(GetFontDefault(), subtitle, subSize, 1);
        BeginDrawing();
        ClearBackground(theme::WINDOW_BG);
        DrawTextEx(GetFontDefault(), title,
                   {(screenW - titleDims.x) / 2.0f,
                    (screenH - titleDims.y) / 2.0f - 20.0f * scale},
                   titleSize, 1, theme::SPLASH_TITLE);
        DrawTextEx(GetFontDefault(), subtitle,
                   {(screenW - subDims.x) / 2.0f,
                    (screenH - subDims.y) / 2.0f + 20.0f * scale},
                   subSize, 1, theme::SPLASH_SUBTITLE);
        EndDrawing();
    }

    std::vector<int> codepoints = LoadFontCodepoints(config::FONT_REGULAR);

    Logger::Info("Loading fonts");
    int scaledFontSize = (int)(config::FONT_SIZE * scale);
    int scaledHeadingSize = (int)(config::FONT_HEADING_SIZE * scale);
    Font bodyFont = LoadFontEx(config::FONT_REGULAR, scaledFontSize,
                               codepoints.data(), (int)codepoints.size());
    SetTextureFilter(bodyFont.texture, TEXTURE_FILTER_POINT);

    Font headingFont = LoadFontEx(config::FONT_REGULAR, scaledHeadingSize,
                                  codepoints.data(), (int)codepoints.size());
    SetTextureFilter(headingFont.texture, TEXTURE_FILTER_POINT);
    Logger::Info("Fonts loaded");

    float headingSize = config::FONT_HEADING_SIZE * scale;
    float contentWidth = screenW - config::CONTENT_PADDING;

#ifdef __ANDROID__
    Logger::Info("Initializing Android asset manager");
    android_app* appState = GetAndroidApp();
    if (!appState) {
        Logger::Error("GetAndroidApp() returned null");
        return 1;
    }
    AAssetManager* mgr = appState->activity->assetManager;
    AndroidAssetProvider androidAssets(mgr);
    IAssetProvider& fileAssets = androidAssets;

    auto envContent = androidAssets.readFileText(config::ENV_FILE);
    if (envContent) {
        EnvLoader::loadFromContent(*envContent);
    }
    Logger::Info("Android assets initialized");
#else
    FileAssetProvider fileAssets;
    EnvLoader::load(config::ENV_FILE);
#endif
    std::string apiKey = EnvLoader::get(config::YVP_APP_KEY);

    Logger::Info("Creating USFM parser");
    USFMParser usfmParser(config::USFM_DIR, &fileAssets);

    std::unique_ptr<IHttpClient> apiClient;
    std::unique_ptr<BibleClient> bibleClient;
    std::unique_ptr<CompositeProvider> compositeProvider;

    ChapterProvider* onlineProv = &usfmParser;
    ChapterProvider* offlineProv = &usfmParser;
    ChapterProvider* activeProv = &usfmParser;

    if (!apiKey.empty()) {
        Logger::Info("API key found, creating online client");
#if defined(__EMSCRIPTEN__)
        apiClient = std::make_unique<EmscriptenClient>();
#elif defined(__ANDROID__)
        apiClient = std::make_unique<AndroidClient>();
#else
        apiClient = std::make_unique<CurlHttpClient>();
#endif
        apiClient->setAppKey(apiKey);
        bibleClient = std::make_unique<BibleClient>(*apiClient, 3034);
        compositeProvider = std::make_unique<CompositeProvider>(*bibleClient, usfmParser);
        onlineProv = bibleClient.get();
        offlineProv = &usfmParser;
        activeProv = compositeProvider.get();
    }

#ifdef __ANDROID__
    std::string dbPath = std::string("/data/data/com.theword.app/app_storage/") + config::DB_FILE;
#else
    std::string home = getenv("HOME") ? getenv("HOME") : ".";
    std::string dbPath = home + "/" + config::DB_DIR + "/" + config::DB_FILE;
#endif
    Logger::Info("Opening database: " + dbPath);
    PersistenceManager storage(dbPath);
    Highlighter highlighter(storage);

    Logger::Info("Database initialized");
    float fontSize = config::FONT_SIZE;
    std::string savedFontSize = storage.getPreference("font_size", "");
    if (!savedFontSize.empty()) {
        fontSize = std::max(config::FONT_SIZE_MIN, std::min(config::FONT_SIZE_MAX, (float)std::atoi(savedFontSize.c_str())));
    }
    float renderedFontSize = fontSize * scale;

    LayoutEngine layoutEngine(contentWidth, bodyFont, renderedFontSize, config::LINE_SPACING, scale);
    Renderer renderer(bodyFont, headingFont, config::TOP_BAR_HEIGHT, renderedFontSize);

    float viewportHeight = screenH - config::TOP_BAR_HEIGHT;
    DocumentManager documentManager(layoutEngine, viewportHeight, *activeProv, config::TOP_BAR_HEIGHT);

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
                        fontSize, versionOnline, scale);

    std::string savedColor = storage.getPreference("active_color", "");
    if (!savedColor.empty()) {
        highlighter.setActiveTypeId(std::atoi(savedColor.c_str()));
    }

    InputHandler inputHandler(documentManager, highlighter, layoutEngine, uiManager, uiManager.getContentTop());

    Logger::Info("Loading initial chapter");
    documentManager.loadInitialChapter("GEN.1");

    double lastTime = GetTime();

    Logger::Info("Entering main loop");
    while (!WindowShouldClose()) {
        double currentTime = GetTime();
        float deltaTime = (float)(currentTime - lastTime);
        lastTime = currentTime;

        inputHandler.handleInput(deltaTime);
        documentManager.update(deltaTime);

        BeginDrawing();
        ClearBackground(theme::WINDOW_BG);

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
        uiManager.drawAbout();
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
