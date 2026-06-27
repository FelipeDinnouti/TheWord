#include "app/App.h"
#include "raylib.h"
#include "core/Config.h"
#include "core/Platform.h"
#include "core/FontHelper.h"
#include "core/BibleBooks.h"
#include "core/IHttpClient.h"
#include "core/Logger.h"
#include "core/EnvLoader.h"
#include "core/Theme.h"
#include "event/EventBus.h"
#include "event/Events.h"
#include "data/USFMParser.h"
#include "data/BibleClient.h"
#include "data/CompositeProvider.h"
#include "text/LayoutEngine.h"
#include "document/DocumentManager.h"
#include "renderer/Renderer.h"
#include "renderer/UIManager.h"
#include "renderer/ContextMenu.h"
#include "input/InputHandler.h"
#include "highlight/Highlighter.h"
#include "persistence/PersistenceManager.h"
#include "ui/NavigationStack.h"
#include "ui/ReaderScreen.h"
#include "ui/SettingsScreen.h"
#include "ui/CreditsOverlay.h"

#include <cstdlib>
#include <algorithm>

namespace theword::app {

using namespace theword::core;
using namespace theword::data;
using namespace theword::text;
using namespace theword::document;
using namespace theword::highlight;
using namespace theword::persistence;
using namespace theword::renderer;
using namespace theword::input;

App::App() = default;
App::~App() {
    UnloadFont(bodyFont_);
    UnloadFont(headingFont_);
    CloseWindow();
}

bool App::Init(const std::string& title) {
    Logger::Info("Starting TheWord");

    auto plat = platform::Init(title.c_str());
    scale_ = plat.dpiScale;

    eventBus_ = std::make_unique<theword::event::EventBus>();

    Logger::Info("Window initialized");
    SetTargetFPS(config::TARGET_FPS);

    int renderW = GetScreenWidth();
    int renderH = GetScreenHeight();

    {
        const char* splashTitle = "TheWord";
        const char* subtitle = "Loading...";
        float titleSize = 48.0f * scale_;
        float subSize = 20.0f * scale_;
        Vector2 titleDims = MeasureTextEx(GetFontDefault(), splashTitle, titleSize, 1);
        Vector2 subDims = MeasureTextEx(GetFontDefault(), subtitle, subSize, 1);
        BeginDrawing();
        ClearBackground(theme::WINDOW_BG);
        DrawTextEx(GetFontDefault(), splashTitle,
                   {(renderW - titleDims.x) / 2.0f,
                    (renderH - titleDims.y) / 2.0f - 20.0f * scale_},
                   titleSize, 1, theme::SPLASH_TITLE);
        DrawTextEx(GetFontDefault(), subtitle,
                   {(renderW - subDims.x) / 2.0f,
                    (renderH - subDims.y) / 2.0f + 20.0f * scale_},
                   subSize, 1, theme::SPLASH_SUBTITLE);
        EndDrawing();
    }

    std::vector<int> codepoints = LoadFontCodepoints(*plat.assets, config::FONT_REGULAR);

    Logger::Info("Loading fonts");
    int scaledFontSize = (int)(config::FONT_SIZE * scale_);
    int scaledHeadingSize = (int)(config::FONT_HEADING_SIZE * scale_);
    bodyFont_ = LoadFontEx(config::FONT_REGULAR, scaledFontSize,
                           codepoints.data(), (int)codepoints.size());
    SetTextureFilter(bodyFont_.texture, TEXTURE_FILTER_POINT);

    headingFont_ = LoadFontEx(config::FONT_REGULAR, scaledHeadingSize,
                              codepoints.data(), (int)codepoints.size());
    SetTextureFilter(headingFont_.texture, TEXTURE_FILTER_POINT);
    Logger::Info("Fonts loaded");

    headingSize_ = config::FONT_HEADING_SIZE * scale_;
    float contentTop = 0.0f;
    float contentWidth = renderW - config::CONTENT_PADDING * scale_;

    std::string apiKey = EnvLoader::get(config::YVP_APP_KEY);

    Logger::Info("Creating USFM parser");
    usfmParser_ = std::make_unique<USFMParser>(config::USFM_DIR, plat.assets.get());
    offlineProv_ = usfmParser_.get();

    if (!apiKey.empty()) {
        Logger::Info("API key found, creating online client");
        apiClient_ = platform::CreateHttpClient();
        if (apiClient_) {
            apiClient_->SetAppKey(apiKey);
            bibleClient_ = std::make_unique<BibleClient>(*apiClient_, 3034);
            compositeProv_ = std::make_unique<CompositeProvider>(*bibleClient_, *usfmParser_);
            onlineProv_ = bibleClient_.get();
            activeProv_ = compositeProv_.get();
        }
    }

    if (activeProv_ == nullptr) {
        activeProv_ = offlineProv_;
    }

    Logger::Info("Opening database: " + plat.dbPath);
    persistence_ = std::make_unique<PersistenceManager>(plat.dbPath);
    highlighter_ = std::make_unique<Highlighter>(*eventBus_, *persistence_);

    Logger::Info("Database initialized");
    currentFontSize_ = config::FONT_SIZE;
    std::string savedFontSize = persistence_->GetPreference("font_size", "");
    if (!savedFontSize.empty()) {
        try { currentFontSize_ = std::max(config::FONT_SIZE_MIN, std::min(config::FONT_SIZE_MAX, (float)std::stoi(savedFontSize))); } catch (...) {}
    }
    float renderedFontSize = currentFontSize_ * scale_;

    layoutEngine_ = std::make_unique<LayoutEngine>(*eventBus_, contentWidth, bodyFont_, renderedFontSize, config::LINE_SPACING, scale_);
    renderer_ = std::make_unique<Renderer>(*eventBus_, bodyFont_, headingFont_, contentTop, renderedFontSize);

    float viewportHeight = renderH - contentTop;
    docManager_ = std::make_unique<DocumentManager>(*eventBus_, *layoutEngine_, viewportHeight, *activeProv_, contentTop);

    if (compositeProv_) {
        std::string savedVersion = persistence_->GetPreference("active_version", "online");
        if (savedVersion == "offline") {
            compositeProv_->SetPrimary(*offlineProv_);
            versionOnline_ = false;
        } else {
            compositeProv_->SetPrimary(*onlineProv_);
            versionOnline_ = true;
        }
    }

    highlighter_->SetProvider(versionOnline_ ? "BibleClient" : "USFMParser");

    uiManager_ = std::make_unique<UIManager>(*eventBus_, headingFont_, headingSize_, *highlighter_, scale_);

    std::string savedColor = persistence_->GetPreference("active_color", "");
    if (!savedColor.empty()) {
        try { highlighter_->SetActiveTypeId(std::stoi(savedColor)); } catch (...) {}
    }

    auto hitTestFn = [this](float x, float y) {
        return docManager_->HitTestWord(x, y, docManager_->GetScrollY());
    };
    auto isHighlightedFn = [this](int wordId) {
        return highlighter_->IsWordHighlighted(wordId);
    };
    inputHandler_ = std::make_unique<InputHandler>(*eventBus_, hitTestFn, isHighlightedFn);

    navStack_ = std::make_unique<theword::ui::NavigationStack>();
    navStack_->Push(std::make_unique<theword::ui::ReaderScreen>(
        *eventBus_, *docManager_, *renderer_, *highlighter_, *persistence_,
        headingFont_, headingSize_, contentTop,
        *navStack_, scale_, currentFontSize_, versionOnline_
    ));

    Logger::Info("Loading initial chapter");
    docManager_->LoadInitialChapter("GEN.1");

    WireEvents();

    Logger::Info("Entering main loop");
    return true;
}

void App::WireEvents() {
    eventBus_->On<theword::event::FontSizeEvent>([this](const auto& e) {
        if (e.delta != 0.0f && e.newSize == 0.0f) {
            float newSz = std::max(config::FONT_SIZE_MIN,
                          std::min(config::FONT_SIZE_MAX, currentFontSize_ + e.delta));
            currentFontSize_ = newSz;
            float scaled = newSz * scale_;
            layoutEngine_->SetFontSize(scaled);
            layoutEngine_->InvalidateCache();
            docManager_->InvalidateLayouts();
            renderer_->SetFontSize(scaled);
        } else if (e.newSize != 0.0f) {
            currentFontSize_ = e.newSize / scale_;
            layoutEngine_->SetFontSize(e.newSize);
            layoutEngine_->InvalidateCache();
            docManager_->InvalidateLayouts();
            renderer_->SetFontSize(e.newSize);
        }
    });

    eventBus_->On<theword::event::SourceSwitchEvent>([this](const auto& e) {
        if (compositeProv_) {
            compositeProv_->SetPrimary(e.online ? *onlineProv_ : *offlineProv_);
        }
        highlighter_->SetProvider(e.online ? "BibleClient" : "USFMParser");
        docManager_->LoadInitialChapter(docManager_->GetCurrentChapterId());
    });

    eventBus_->On<theword::event::RightClickEvent>([this](const auto& e) {
        float scrollY = docManager_->GetScrollY();
        int wordId = docManager_->HitTestWord(e.x, e.y, scrollY);
        if (wordId >= 0) {
            const Highlight* hl = highlighter_->HighlightAtWord(wordId);
            if (hl) {
                uiManager_->ShowContextMenu({e.x, e.y}, hl->id, hl->typeId);
            }
        }
    });

    eventBus_->On<theword::event::NavigateEvent>([this](const auto& e) {
        docManager_->LoadInitialChapter(e.chapterRef);
    });
}

void App::Run() {
    double lastTime = GetTime();

    while (!WindowShouldClose()) {
        if (platform::ShouldQuit()) break;

        double currentTime = GetTime();
        float deltaTime = (float)(currentTime - lastTime);
        lastTime = currentTime;

        auto ctxHandler = [this](Vector2 pos) {
            return uiManager_->HandleContextMenuClick(pos);
        };
        auto ctxDismiss = [this]() {
            if (uiManager_->IsContextMenuActive()) {
                uiManager_->HideContextMenu();
                return true;
            }
            return false;
        };
        inputHandler_->Poll(deltaTime, navStack_.get(), ctxHandler, ctxDismiss);
        docManager_->Update(deltaTime);

        // Keyboard shortcuts that push screens (only when on root Reader)
        if (navStack_->IsOnRoot()) {
            if (IsKeyPressed(key::S)) {
                navStack_->Push(std::make_unique<theword::ui::SettingsScreen>(
                    headingFont_, headingSize_, *navStack_, *eventBus_,
                    *highlighter_, *persistence_,
                    scale_, currentFontSize_, versionOnline_
                ));
            }
            if (IsKeyPressed(key::A)) {
                navStack_->Push(std::make_unique<theword::ui::CreditsOverlay>(
                    headingFont_, headingSize_, *navStack_
                ));
            }
        }

        BeginDrawing();
        ClearBackground(theme::WINDOW_BG);

        navStack_->DrawActive();

        uiManager_->DrawContextMenu();
#ifndef NDEBUG
        renderer_->DrawFpsCounter(10, GetScreenHeight() - 30);
#endif

        EndDrawing();
    }
}

} // namespace theword::app
