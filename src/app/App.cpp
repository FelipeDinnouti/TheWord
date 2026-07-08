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
#include "core/Locale.h"
#include "event/EventBus.h"
#include "event/Events.h"
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
#include "ui/NavigationStack.h"
#include "ui/ReaderScreen.h"
#include "ui/SettingsScreen.h"
#include "ui/CreditsOverlay.h"
#include "ui/FontDiagnostic.h"

#include <cstdlib>
#include <algorithm>
#include <sstream>

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
    UnloadFont(largeFont_);
    UnloadFont(smallFont_);
    CloseWindow();
}

static std::string AssembleSelectedText(const ChapterData& data, int startWord, int endWord) {
    int s = (std::min)(startWord, endWord);
    int e = (std::max)(startWord, endWord);
    std::ostringstream oss;
    for (const auto& w : data.words) {
        if (w.id >= s && w.id <= e) {
            if (oss.tellp() > 0) oss << " ";
            oss << w.text;
        }
    }
    return oss.str();
}

static void FindVerseRange(const std::vector<Word>& words, int anchorWord, int& verseStart, int& verseEnd) {
    int targetVerse = -1;
    for (const auto& w : words) {
        if (w.id == anchorWord) {
            targetVerse = w.verseId;
            break;
        }
    }
    if (targetVerse < 0) { verseStart = anchorWord; verseEnd = anchorWord; return; }

    verseStart = anchorWord;
    verseEnd = anchorWord;
    for (const auto& w : words) {
        if (w.verseId == targetVerse) {
            if (w.id < verseStart) verseStart = w.id;
            if (w.id > verseEnd) verseEnd = w.id;
        }
    }
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
    uiScale_ = {scale_, (float)renderW, (float)renderH, (float)plat.bottomInset};

    {
        const char* splashTitle = "TheWord";
        const char* subtitle = Locale::Get("Loading...");
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

    fontCodepoints_ = LoadFontCodepoints(*plat.assets, config::FONT_REGULAR);

    Logger::Info("Opening database: " + plat.dbPath);
    persistence_ = std::make_unique<PersistenceManager>(plat.dbPath);
    highlighter_ = std::make_unique<Highlighter>(*eventBus_, *persistence_);

    Logger::Info("Database initialized");
    currentFontSize_ = config::FONT_SIZE;
    std::string savedFontSize = persistence_->GetPreference("font_size", "");
    if (!savedFontSize.empty()) {
        try { currentFontSize_ = std::max(config::FONT_SIZE_MIN, std::min(config::FONT_SIZE_MAX, (float)std::stoi(savedFontSize))); } catch (...) {}
    }

    Logger::Info("Loading fonts");
    ReloadFonts(currentFontSize_, fontCodepoints_);
    Logger::Info("Fonts loaded");

    headingSize_ = currentFontSize_ * theme::FONT_HEADING * scale_;
    float contentTop = 0.0f;
    float contentWidth = static_cast<float>(renderW);

    int initBody = (int)(currentFontSize_ * scale_);
    float initBodyF = (float)std::max(1, initBody);
    float initHeadingF = (float)std::max(1, (int)(currentFontSize_ * theme::FONT_HEADING * scale_));
    float initLargeF = (float)std::max(1, (int)(currentFontSize_ * theme::FONT_LARGE_HEADING * scale_));
    float initSmallF = (float)std::max(1, (int)(currentFontSize_ * theme::FONT_VERSE_NUMBER * scale_));

    layoutEngine_ = std::make_unique<LayoutEngine>(*eventBus_, contentWidth,
                                                   bodyFont_, initBodyF,
                                                   headingFont_, initHeadingF,
                                                   largeFont_, initLargeF,
                                                   smallFont_, initSmallF,
                                                   config::LINE_SPACING, scale_);
    renderer_ = std::make_unique<Renderer>(*eventBus_, bodyFont_, headingFont_, largeFont_, smallFont_,
                                            contentTop, initBodyF, initHeadingF, initLargeF, initSmallF, scale_);

    float viewportHeight = renderH - contentTop;

    std::string apiKey = EnvLoader::get(config::YVP_APP_KEY);

    Logger::Info("Creating USFM parser");
    usfmParser_ = std::make_unique<USFMParser>(config::USFM_DIR, std::move(plat.assets));
    offlineProv_ = usfmParser_.get();

    if (!apiKey.empty()) {
        Logger::Info("API key found, creating online client");
        apiClient_ = platform::CreateHttpClient();
        if (apiClient_) {
            apiClient_->SetAppKey(apiKey);
            bibleClient_ = std::make_unique<BibleClient>(*apiClient_, 129);
            compositeProv_ = std::make_unique<CompositeProvider>(*bibleClient_, *usfmParser_);
            onlineProv_ = bibleClient_.get();
            activeProv_ = compositeProv_.get();
        }
    }

    if (activeProv_ == nullptr) {
        activeProv_ = offlineProv_;
    }

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
        *navStack_, uiScale_, currentFontSize_, versionOnline_
    ));

    {
        std::string startChapter = persistence_->GetPreference("last_chapter", "GEN.1");
        Logger::Info("Loading initial chapter: " + startChapter);
        docManager_->LoadInitialChapterSync(startChapter);
        auto* chapterData = docManager_->GetCurrentChapterData();
        std::string book;
        int chapter = 0;
        ParseChapterRef(startChapter, book, chapter);
        highlighter_->SetChapterContext(book, chapter,
            chapterData ? &chapterData->words : nullptr);
    }

    WireEvents();

    Logger::Info("Entering main loop");
    return true;
}

void App::ReloadFonts(float newFontSize, std::vector<int>& codepoints) {
    int scaledFontSize = (int)(newFontSize * scale_);
    scaledFontSize = std::max(1, scaledFontSize);
    int scaledHeadingSize = (int)(newFontSize * theme::FONT_HEADING * scale_);
    scaledHeadingSize = std::max(1, scaledHeadingSize);
    int scaledLargeSize = (int)(newFontSize * theme::FONT_LARGE_HEADING * scale_);
    scaledLargeSize = std::max(1, scaledLargeSize);
    int scaledSmallSize = (int)(newFontSize * theme::FONT_VERSE_NUMBER * scale_);
    scaledSmallSize = std::max(1, scaledSmallSize);

    Font newBody = LoadFontEx(config::FONT_REGULAR, scaledFontSize,
                              codepoints.data(), (int)codepoints.size());
    SetTextureFilter(newBody.texture, TEXTURE_FILTER_POINT);

    Font newHeading = LoadFontEx(config::FONT_REGULAR, scaledHeadingSize,
                                 codepoints.data(), (int)codepoints.size());
    SetTextureFilter(newHeading.texture, TEXTURE_FILTER_BILINEAR);

    Font newLarge = LoadFontEx(config::FONT_REGULAR, scaledLargeSize,
                               codepoints.data(), (int)codepoints.size());
    SetTextureFilter(newLarge.texture, TEXTURE_FILTER_POINT);

    Font newSmall = LoadFontEx(config::FONT_REGULAR, scaledSmallSize,
                               codepoints.data(), (int)codepoints.size());
    SetTextureFilter(newSmall.texture, TEXTURE_FILTER_POINT);

    Font newBold = LoadFontEx(config::FONT_BOLD, scaledFontSize,
                              codepoints.data(), (int)codepoints.size());
    SetTextureFilter(newBold.texture, TEXTURE_FILTER_POINT);

    headingSize_ = config::FONT_HEADING_SIZE / config::FONT_SIZE * newFontSize * scale_;

    Font oldBody = bodyFont_;
    Font oldHeading = headingFont_;
    Font oldLarge = largeFont_;
    Font oldSmall = smallFont_;
    Font oldBold = boldFont_;
    bodyFont_ = newBody;
    headingFont_ = newHeading;
    largeFont_ = newLarge;
    smallFont_ = newSmall;
    boldFont_ = newBold;

    UnloadFont(oldBody);
    UnloadFont(oldHeading);
    UnloadFont(oldLarge);
    UnloadFont(oldSmall);
    UnloadFont(oldBold);
}

void App::WireEvents() {
    eventBus_->On<theword::event::FontSizeEvent>([this](const auto& e) {
        float newSize;
        if (e.delta != 0.0f && e.newSize == 0.0f) {
            newSize = std::max(config::FONT_SIZE_MIN,
                      std::min(config::FONT_SIZE_MAX, currentFontSize_ + e.delta));
        } else if (e.newSize != 0.0f) {
            newSize = e.newSize / scale_;
        } else {
            return;
        }

        if (newSize == currentFontSize_) return;
        currentFontSize_ = newSize;

        ReloadFonts(newSize, fontCodepoints_);

        float bodyF = (float)std::max(1, (int)(newSize * scale_));
        float headingF = (float)std::max(1, (int)(newSize * theme::FONT_HEADING * scale_));
        float largeF = (float)std::max(1, (int)(newSize * theme::FONT_LARGE_HEADING * scale_));
        float smallF = (float)std::max(1, (int)(newSize * theme::FONT_VERSE_NUMBER * scale_));
        layoutEngine_->SetFontSizes(bodyF, headingF, largeF, smallF);
        layoutEngine_->InvalidateCache();
        docManager_->InvalidateLayouts();
        renderer_->SetFontSizes(bodyF, headingF, largeF, smallF);
    });

    eventBus_->On<theword::event::SourceSwitchEvent>([this](const auto& e) {
        if (compositeProv_) {
            compositeProv_->SetPrimary(e.online ? *onlineProv_ : *offlineProv_);
        }
        highlighter_->SetProvider(e.online ? "BibleClient" : "USFMParser");
        docManager_->LoadInitialChapter(docManager_->GetCurrentChapterId());
    });

    eventBus_->On<theword::event::NavigateEvent>([this](const auto& e) {
        docManager_->LoadInitialChapter(e.chapterRef);
        persistence_->SetPreference("last_chapter", e.chapterRef);
    });

    eventBus_->On<theword::event::ChapterLoadedEvent>([this](const auto& e) {
        std::string book;
        int chapter;
        if (ParseChapterRef(e.chapterRef, book, chapter)) {
            auto* chapterData = docManager_->GetCurrentChapterData();
            highlighter_->SetChapterContext(book, chapter,
                chapterData ? &chapterData->words : nullptr);
        }
    });

    eventBus_->On<theword::event::ResizeEvent>([this](const auto& e) {
        uiScale_.OnResize((float)e.width, (float)e.height);
    });
}

void App::Run() {
    double lastTime = GetTime();
    int drawCountdown = config::IDLE_DRAIN_FRAMES;

    while (!WindowShouldClose()) {
        if (platform::ShouldQuit()) break;

        double currentTime = GetTime();
        float deltaTime = (float)(currentTime - lastTime);
        lastTime = currentTime;

        auto radialClickHandler = [this](Vector2 pos) -> bool {
            RadialMenuActionResult result = uiManager_->HandleRadialMenuClick(pos);
            if (!result.consumed) return false;

            if (result.isCopy) {
                auto* chapterData = docManager_->GetCurrentChapterData();
                if (chapterData) {
                    std::string text = AssembleSelectedText(*chapterData,
                        result.startWord, result.endWord);
                    if (!text.empty()) {
                        platform::SetClipboard(text);
                        Logger::Debug("Copied to clipboard: " + text);
                    }
                }
            } else if (result.isDelete) {
                auto* chapterData = docManager_->GetCurrentChapterData();
                if (chapterData) {
                    int s = (std::min)(result.startWord, result.endWord);
                    int e = (std::max)(result.startWord, result.endWord);
                    for (const auto& h : highlighter_->GetHighlights()) {
                        if (h.bookId == chapterData->bookId &&
                            h.chapterNum == chapterData->chapterNum &&
                            h.startWord <= e && h.endWord >= s) {
                            highlighter_->RemoveHighlight(h.id);
                        }
                    }
                }
            }

            return true;
        };

        auto radialDismiss = [this]() -> bool {
            if (uiManager_->IsRadialMenuActive()) {
                uiManager_->HideRadialMenu();
                return true;
            }
            return false;
        };

        auto radialShowHandler = [this](int startWord, int endWord, Vector2 position, bool selectFullVerse) {
            auto* chapterData = docManager_->GetCurrentChapterData();
            if (!chapterData) return;

            if (selectFullVerse) {
                int vStart, vEnd;
                FindVerseRange(chapterData->words, startWord, vStart, vEnd);
                startWord = vStart;
                endWord = vEnd;
            }

            uiManager_->ShowRadialMenu(position, startWord, endWord);
        };

        inputHandler_->Poll(deltaTime, navStack_.get(), radialClickHandler, radialDismiss, radialShowHandler);
        docManager_->Update(deltaTime);

        // Keyboard shortcuts that push screens (only when on root Reader)
        if (navStack_->IsOnRoot()) {
            if (IsKeyPressed(key::S)) {
                navStack_->Push(std::make_unique<theword::ui::SettingsScreen>(
                    headingFont_, headingSize_, *navStack_, *eventBus_,
                    *highlighter_, *persistence_,
                    uiScale_, currentFontSize_, versionOnline_
                ));
            }
            if (IsKeyPressed(key::A)) {
                navStack_->Push(std::make_unique<theword::ui::CreditsOverlay>(
                    headingFont_, headingSize_, *navStack_, uiScale_
                ));
            }
            if (IsKeyPressed(KEY_D)) {
                navStack_->Push(std::make_unique<theword::ui::FontDiagnostic>(
                    bodyFont_, headingFont_, largeFont_, smallFont_, boldFont_,
                    scale_, *navStack_
                ));
            }
        }

        // Skip GPU draws when the view is fully static
        bool isAnimating = inputHandler_->HasMomentum()
                        || docManager_->HasMomentum()
                        || docManager_->HasPendingLoads();
        bool hasUiOverlay = inputHandler_->IsDialogActive()
                         || uiManager_->IsRadialMenuActive()
                         || !navStack_->IsOnRoot();

        static int idleFrameCount = 0;
        if (isAnimating || hasUiOverlay) {
            drawCountdown = config::IDLE_DRAIN_FRAMES;
            idleFrameCount = 0;
        } else if (drawCountdown > 0) {
            drawCountdown--;
        }

        bool doDraw = false;
        if (drawCountdown > 0) {
            doDraw = true;
        } else {
            if (++idleFrameCount >= config::IDLE_DRAIN_INTERVAL) {
                idleFrameCount = 0;
                doDraw = true;
            }
        }

        if (doDraw) {
            BeginDrawing();
            ClearBackground(theme::WINDOW_BG);

            navStack_->DrawActive();

            uiManager_->DrawRadialMenu();
#ifndef NDEBUG
            renderer_->DrawFpsCounter(10, GetScreenHeight() - 30);
#endif

            EndDrawing();
        }
    }
}

} // namespace theword::app
