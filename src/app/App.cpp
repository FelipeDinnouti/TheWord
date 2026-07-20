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
#include "core/ThemeManager.h"
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
    if (s < 0 || e >= static_cast<int>(data.words.size())) return {};

    // Determine verse range
    int firstVerse = data.words[s].verseId;
    int lastVerse = data.words[e].verseId;

    // Build citation prefix
    int idx = FindBookIndex(data.bookId);
    std::string bookName = (idx >= 0) ? BOOK_NAMES_PT[idx] : data.bookId;
    std::ostringstream citation;
    citation << bookName << " " << data.chapterNum << ":" << firstVerse;
    if (lastVerse > firstVerse) citation << "-" << lastVerse;
    citation << "\n\n";

    // Build body text
    std::ostringstream body;
    for (const auto& w : data.words) {
        if (w.id >= s && w.id <= e) {
            if (body.tellp() > 0) body << " ";
            body << w.text;
        }
    }
    return citation.str() + body.str();
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

    themeManager_ = std::make_unique<ThemeManager>();

    {
        const auto& pal = themeManager_->Current();
        const char* splashTitle = "TheWord";
        const char* subtitle = Locale::Get("Loading...");
        float titleSize = 48.0f * scale_;
        float subSize = 20.0f * scale_;
        Vector2 titleDims = MeasureTextEx(GetFontDefault(), splashTitle, titleSize, 1);
        Vector2 subDims = MeasureTextEx(GetFontDefault(), subtitle, subSize, 1);
        BeginDrawing();
        ClearBackground(pal.windowBg);
        DrawTextEx(GetFontDefault(), splashTitle,
                   {(renderW - titleDims.x) / 2.0f,
                    (renderH - titleDims.y) / 2.0f - 20.0f * scale_},
                   titleSize, 1, pal.splashTitle);
        DrawTextEx(GetFontDefault(), subtitle,
                   {(renderW - subDims.x) / 2.0f,
                    (renderH - subDims.y) / 2.0f + 20.0f * scale_},
                   subSize, 1, pal.splashSubtitle);
        EndDrawing();
    }

    fontCodepoints_ = LoadFontCodepoints(*plat.assets, config::FONT_REGULAR);

    Logger::Info("Opening database: " + plat.dbPath);
    persistence_ = std::make_unique<PersistenceManager>(plat.dbPath);
    highlighter_ = std::make_unique<Highlighter>(*eventBus_, *persistence_);

    std::string savedDarkMode = persistence_->GetPreference("dark_mode", "0");
    if (savedDarkMode == "1") themeManager_->SetDarkMode(true);

    Logger::Info("Database initialized");
    currentFontSize_ = config::FONT_SIZE;
    std::string savedFontSize = persistence_->GetPreference("font_size", "");
    if (!savedFontSize.empty()) {
        try { currentFontSize_ = std::max(config::FONT_SIZE_MIN, std::min(config::FONT_SIZE_MAX, (float)std::stoi(savedFontSize))); } catch (...) {}
    }

    std::string savedBibleId = persistence_->GetPreference("bible_id", std::to_string(config::DEFAULT_BIBLE_ID));
    try { currentBibleId_ = std::stoi(savedBibleId); } catch (...) { currentBibleId_ = config::DEFAULT_BIBLE_ID; }

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
    renderer_ = std::make_unique<Renderer>(bodyFont_, headingFont_, largeFont_, smallFont_,
                                            contentTop, initBodyF, initHeadingF, initLargeF, initSmallF, scale_,
                                            *themeManager_);

    float viewportHeight = renderH - contentTop;

    std::string apiKey = EnvLoader::Get(config::YVP_APP_KEY);

    Logger::Info("Creating USFM parser");
    usfmParser_ = std::make_unique<USFMParser>(config::USFM_DIR, std::move(plat.assets));

    if (!apiKey.empty()) {
        Logger::Info("API key found, creating online client");
        apiClient_ = platform::CreateHttpClient();
        if (apiClient_) {
            apiClient_->SetAppKey(apiKey);
            if (currentBibleId_ > 0) {
                bibleClient_ = std::make_unique<BibleClient>(*apiClient_, currentBibleId_);
                compositeProv_ = std::make_unique<CompositeProvider>(*bibleClient_);
            } else {
                compositeProv_ = std::make_unique<CompositeProvider>(*usfmParser_);
            }
            activeProv_ = compositeProv_.get();
        }
    }

    if (activeProv_ == nullptr) {
        compositeProv_ = std::make_unique<CompositeProvider>(*usfmParser_);
        activeProv_ = compositeProv_.get();
    }

    docManager_ = std::make_unique<DocumentManager>(*eventBus_, *layoutEngine_, viewportHeight, *activeProv_, contentTop);

    highlighter_->SetProvider(currentBibleId_ > 0 ? "BibleClient" : "USFMParser");

    uiManager_ = std::make_unique<UIManager>(*highlighter_, smallFont_, *themeManager_, scale_);

    {
        std::string im = persistence_->GetPreference("immersive_mode", "0");
        immersiveMode_ = (im == "1");
    }

    std::string savedColor = persistence_->GetPreference("active_color", "");
    if (!savedColor.empty()) {
        try { highlighter_->SetActiveTypeId(std::stoi(savedColor)); } catch (...) {}
    }

    auto hitTestFn = [this](float x, float y) -> theword::input::HitInfo {
        if (!navStack_->IsOnRoot()) return {};
        auto result = docManager_->HitTestWithChapter(x, y);
        return {result.wordId,
                result.chapterData ? result.chapterData->bookId : "",
                result.chapterData ? result.chapterData->chapterNum : 0};
    };
    auto isHighlightedFn = [this](int wordId) {
        return highlighter_->IsWordHighlighted(wordId);
    };
    auto hitTestFootnoteFn = [this, contentTop](float x, float y) -> int {
        std::vector<std::pair<theword::data::Span, float>> spans;
        docManager_->GetVisibleSpans(spans);
        for (const auto& [span, docY] : spans) {
            if (span.type != SegmentType::FootnoteMarker || span.footnoteIndex < 0) continue;
            float screenY = docY - docManager_->GetScrollY() + contentTop;
            if (y >= screenY && y <= screenY + span.height &&
                x >= span.x && x <= span.x + span.width) {
                return span.footnoteIndex;
            }
        }
        return -1;
    };
    inputHandler_ = std::make_unique<InputHandler>(*eventBus_, hitTestFn, isHighlightedFn, hitTestFootnoteFn);

    navStack_ = std::make_unique<theword::ui::NavigationStack>();
    navStack_->Push(std::make_unique<theword::ui::ReaderScreen>(
        *eventBus_, *docManager_, *renderer_, *highlighter_, *persistence_,
        headingFont_, headingSize_, contentTop,
        *navStack_, uiScale_, currentFontSize_, currentBibleId_, immersiveMode_,
        *themeManager_
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

        // Restore scroll position from pre-pause save (OS kill recovery)
        std::string savedScroll = persistence_->GetPreference("lifecycle_scroll", "");
        if (!savedScroll.empty()) {
            docManager_->ScrollTo(std::stof(savedScroll));
            persistence_->SetPreference("lifecycle_scroll", "");
        }

        // Load last known scroll for deferred restore on first MainLoop frame
        std::string lastScroll = persistence_->GetPreference("last_scroll", "");
        if (!lastScroll.empty()) {
            pendingScrollY_ = std::stof(lastScroll);
        }
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

    eventBus_->On<theword::event::BibleVersionSwitchEvent>([this](const auto& e) {
        currentBibleId_ = e.bibleId;
        persistence_->SetPreference("bible_id", std::to_string(e.bibleId));

        if (e.bibleId == 0) {
            compositeProv_->SetPrimary(*usfmParser_);
        } else {
            bibleClient_.reset();
            bibleClient_ = std::make_unique<BibleClient>(*apiClient_, e.bibleId);
            compositeProv_->SetPrimary(*bibleClient_);
        }

        highlighter_->SetProvider(e.bibleId == 0 ? "USFMParser" : "BibleClient");
        layoutEngine_->InvalidateCache();
        docManager_->LoadInitialChapter(docManager_->GetCurrentChapterId());
    });

    eventBus_->On<theword::event::ThemeToggleEvent>([this](const auto&) {
        themeManager_->Toggle();
        persistence_->SetPreference("dark_mode", themeManager_->IsDarkMode() ? "1" : "0");
    });

    eventBus_->On<theword::event::NavigateEvent>([this](const auto& e) {
        persistence_->SetPreference("last_scroll",
            std::to_string(docManager_->GetScrollY()));
        docManager_->LoadInitialChapter(e.chapterRef);
        persistence_->SetPreference("last_chapter", e.chapterRef);
    });

    eventBus_->On<theword::event::ChapterLoadedEvent>([this](const auto& e) {
        accumStartWord_ = -1;
        accumEndWord_ = -1;
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

void App::WireInputCallbacks() {
    inputHandler_->onTap = [this](theword::input::HitInfo hit, Vector2 pos, bool isDouble) {
        OnTap(hit, pos, isDouble);
    };
    inputHandler_->onTapEmpty = [this](Vector2 pos) {
        OnTapEmpty(pos);
    };
    inputHandler_->onDragStart = [this](int startWord, Vector2 pos) {
        OnDragStart(startWord, pos);
    };
    inputHandler_->onDragUpdate = [this](int startWord, int currentWord, Vector2 pos) {
        OnDragUpdate(startWord, currentWord, pos);
    };
    inputHandler_->onDragEnd = [this](int startWord, int endWord, Vector2 pos) {
        OnDragEnd(startWord, endWord, pos);
    };
    inputHandler_->onLongPress = [this](int wordId, Vector2 pos) {
        OnLongPress(wordId, pos);
    };
    inputHandler_->onFootnoteTap = [this](int fi) {
        auto* cd = docManager_->GetCurrentChapterData();
        if (cd && fi >= 0 && fi < static_cast<int>(cd->footnotes.size())) {
            const auto& fn = cd->footnotes[fi];
            std::string text = fn.callerRef.empty() ? fn.text : fn.callerRef + " " + fn.text;
            uiManager_->ShowFootnotePopup(text, GetMousePosition());
        }
    };
    inputHandler_->onDismiss = [this]() -> bool {
        return OnDismiss();
    };
}

void App::HandleShortcuts() {
    if (!navStack_->IsOnRoot()) return;

    if (IsKeyPressed(key::S)) {
        navStack_->Push(std::make_unique<theword::ui::SettingsScreen>(
            headingFont_, headingSize_, *navStack_, *eventBus_,
            *persistence_,
            uiScale_, currentFontSize_, currentBibleId_, immersiveMode_,
            *themeManager_
        ));
    }
    if (IsKeyPressed(key::A)) {
        navStack_->Push(std::make_unique<theword::ui::CreditsOverlay>(
            headingFont_, headingSize_, *navStack_, uiScale_,
            *themeManager_
        ));
    }
    if (IsKeyPressed(key::I)) {
        immersiveMode_ = !immersiveMode_;
        persistence_->SetPreference("immersive_mode", immersiveMode_ ? "1" : "0");
    }
    if (IsKeyPressed(KEY_D)) {
        navStack_->Push(std::make_unique<theword::ui::FontDiagnostic>(
            bodyFont_, headingFont_, largeFont_, smallFont_, boldFont_,
            scale_, *navStack_,
            *themeManager_
        ));
    }
    if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_C) && accumStartWord_ >= 0) {
        auto* cd = docManager_->GetCurrentChapterData();
        if (cd) CopySelection(*cd, accumStartWord_, accumEndWord_);
    }
}

void App::MainLoop() {
    double lastTime = GetTime();
    int drawCountdown = config::IDLE_COOLDOWN_FRAMES;

#if defined(__ANDROID__)
    bool wasWindowGone = false;
#endif

    while (!WindowShouldClose()) {
        if (pendingScrollY_ >= 0.0f) {
            docManager_->ScrollTo(pendingScrollY_);
            pendingScrollY_ = -1.0f;
        }

        if (platform::ShouldQuit()) {
            if (navStack_->IsOnRoot()) {
                persistence_->SetPreference("last_scroll",
                    std::to_string(docManager_->GetScrollY()));
            }
            break;
        }

#if defined(__ANDROID__)
        if (!platform::IsWindowAvailable()) {
            PollInputEvents();
            if (platform::ShouldQuit()) break;
            if (!wasWindowGone) {
                wasWindowGone = true;
                float sy = docManager_->GetScrollY();
                persistence_->SetPreference("lifecycle_scroll", std::to_string(sy));
                persistence_->SetPreference("last_scroll", std::to_string(sy));
            }
            WaitTime(0.016);
            continue;
        }
        if (wasWindowGone) {
            wasWindowGone = false;
            inputHandler_->ResetState();
        }
#endif

        double currentTime = GetTime();
        float deltaTime = (float)(currentTime - lastTime);
        lastTime = currentTime;

        if (!navStack_->HandleInput(deltaTime)) {
            inputHandler_->Poll(deltaTime);
        }
        docManager_->Update(deltaTime);

        HandleShortcuts();

        bool isAnimating = inputHandler_->HasMomentum()
                        || docManager_->HasMomentum()
                        || docManager_->HasPendingLoads();
        bool hasUiOverlay = inputHandler_->IsDialogActive()
                         || uiManager_->IsRadialMenuActive()
                         || !navStack_->IsOnRoot();
        bool hasActiveInput = GetTouchPointCount() > 0;

        static double lastIdleDrawTime = 0.0;

        if (isAnimating || hasUiOverlay || hasActiveInput) {
            drawCountdown = config::IDLE_COOLDOWN_FRAMES;
        } else if (drawCountdown > 0) {
            drawCountdown--;
            if (drawCountdown == 0) lastIdleDrawTime = 0.0;
        }

        bool doDraw = false;
        if (drawCountdown > 0) {
            doDraw = true;
        } else {
            doDraw = (currentTime - lastIdleDrawTime) >= config::IDLE_DRAW_INTERVAL;
            if (doDraw) lastIdleDrawTime = currentTime;
        }

        if (doDraw) {
            const auto& pal = themeManager_->Current();
            BeginDrawing();
            ClearBackground(pal.windowBg);

            navStack_->DrawActive();

            uiManager_->DrawRadialMenu();
            uiManager_->DrawToast();
            uiManager_->DrawFootnotePopup();
            renderer_->DrawFpsCounter(GetScreenWidth() / 2, GetScreenHeight() - 30);

            EndDrawing();
        } else {
            PollInputEvents();
        }
    }
}

void App::OnTap(theword::input::HitInfo hit, Vector2 pos, bool isDouble) {
    if (uiManager_->IsFootnotePopupActive()) {
        uiManager_->HideFootnotePopup();
        return;
    }
    int wordId = hit.wordId;
    auto* cd = docManager_->GetChapterData(hit.bookId, hit.chapterNum);
    if (!cd) cd = docManager_->GetCurrentChapterData();
    Logger::Debug("onTap: wordId=" + std::to_string(hit.wordId)
        + " hitChapter=" + hit.bookId + "." + std::to_string(hit.chapterNum)
        + " visibleChapter=" + docManager_->GetCurrentChapterId()
        + " cd=" + (cd ? cd->bookId + "." + std::to_string(cd->chapterNum) : "null"));

    if (uiManager_->IsRadialMenuActive()) {
        RadialMenuActionResult result = uiManager_->HandleRadialMenuClick(pos);
        if (result.consumed) {
            accumStartWord_ = -1;
            accumEndWord_ = -1;
            if (result.isCopy) {
                if (cd) CopySelection(*cd, result.startWord, result.endWord);
            } else if (result.isDelete) {
                if (cd) {
                    int s = (std::min)(result.startWord, result.endWord);
                    int e = (std::max)(result.startWord, result.endWord);
                    for (const auto& h : highlighter_->GetHighlights()) {
                        if (h.bookId == cd->bookId &&
                            h.chapterNum == cd->chapterNum &&
                            h.startWord <= e && h.endWord >= s) {
                            highlighter_->RemoveHighlight(h.id);
                        }
                    }
                }
            } else if (result.isHighlight) {
                const auto& types = highlighter_->GetTypes();
                if (result.colorIndex >= 0 && result.colorIndex < static_cast<int>(types.size())) {
                    const auto* existing = highlighter_->HighlightOverlapping(result.startWord, result.endWord, result.bookId, result.chapterNum);
                    if (existing) {
                        highlighter_->RecolorHighlight(existing->id, types[result.colorIndex].id);
                    } else {
                        auto* hcd = docManager_->GetChapterData(result.bookId, result.chapterNum);
                        highlighter_->CreateHighlight(result.startWord, result.endWord, types[result.colorIndex].id,
                            result.bookId, result.chapterNum, hcd ? &hcd->words : nullptr);
                    }
                }
                uiManager_->HideRadialMenu();
            }
            return;
        }
        if (wordId >= 0 && accumStartWord_ >= 0 && cd) {
            int vStart, vEnd;
            FindVerseRange(cd->words, wordId, vStart, vEnd);
            accumStartWord_ = (std::min)(accumStartWord_, vStart);
            accumEndWord_ = (std::max)(accumEndWord_, vEnd);
            highlighter_->CommitSelection(accumStartWord_, accumEndWord_, cd->bookId, cd->chapterNum);
            uiManager_->ShowRadialMenu(pos, accumStartWord_, accumEndWord_, cd->bookId, cd->chapterNum);
            return;
        }
        if (wordId >= 0) {
            const auto* h = highlighter_->HighlightAtWord(wordId, hit.bookId, hit.chapterNum);
            if (h) {
                accumStartWord_ = -1;
                accumEndWord_ = -1;
                highlighter_->CommitSelection(h->startWord, h->endWord, h->bookId, h->chapterNum);
                uiManager_->ShowRadialMenu(pos, h->startWord, h->endWord, h->bookId, h->chapterNum);
            } else {
                uiManager_->HideRadialMenu();
                accumStartWord_ = -1;
                accumEndWord_ = -1;
            }
            return;
        }
        uiManager_->HideRadialMenu();
        accumStartWord_ = -1;
        accumEndWord_ = -1;
        return;
    }

    if (!cd || wordId < 0) return;

    if (isDouble) {
        int vStart, vEnd;
        FindVerseRange(cd->words, wordId, vStart, vEnd);
        accumStartWord_ = accumStartWord_ >= 0
            ? (std::min)(accumStartWord_, vStart) : vStart;
        accumEndWord_ = accumEndWord_ >= 0
            ? (std::max)(accumEndWord_, vEnd) : vEnd;
        highlighter_->CommitSelection(accumStartWord_, accumEndWord_, hit.bookId, hit.chapterNum);
        uiManager_->ShowRadialMenu(pos, accumStartWord_, accumEndWord_, hit.bookId, hit.chapterNum);
    } else if (auto* h = highlighter_->HighlightAtWord(wordId, hit.bookId, hit.chapterNum)) {
        accumStartWord_ = -1;
        accumEndWord_ = -1;
        highlighter_->CommitSelection(h->startWord, h->endWord, h->bookId, h->chapterNum);
        uiManager_->ShowRadialMenu(pos, h->startWord, h->endWord, h->bookId, h->chapterNum);
    }
}

void App::OnTapEmpty(Vector2 pos) {
    if (uiManager_->IsFootnotePopupActive()) {
        uiManager_->HideFootnotePopup();
        return;
    }
    if (uiManager_->IsRadialMenuActive()) {
        RadialMenuActionResult result = uiManager_->HandleRadialMenuClick(pos);
        if (result.consumed) {
            accumStartWord_ = -1;
            accumEndWord_ = -1;
            if (result.isCopy) {
                auto* cd = docManager_->GetCurrentChapterData();
                if (cd) CopySelection(*cd, result.startWord, result.endWord);
            } else if (result.isDelete) {
                int s = (std::min)(result.startWord, result.endWord);
                int e = (std::max)(result.startWord, result.endWord);
                for (const auto& h : highlighter_->GetHighlights()) {
                    if (h.bookId == result.bookId && h.chapterNum == result.chapterNum
                        && h.startWord <= e && h.endWord >= s) {
                        highlighter_->RemoveHighlight(h.id);
                    }
                }
            } else if (result.isHighlight) {
                const auto& types = highlighter_->GetTypes();
                if (result.colorIndex >= 0 && result.colorIndex < static_cast<int>(types.size())) {
                    const auto* existing = highlighter_->HighlightOverlapping(result.startWord, result.endWord,
                        result.bookId, result.chapterNum);
                    if (existing) {
                        highlighter_->RecolorHighlight(existing->id, types[result.colorIndex].id);
                    } else {
                        auto* hcd = docManager_->GetChapterData(result.bookId, result.chapterNum);
                        highlighter_->CreateHighlight(result.startWord, result.endWord, types[result.colorIndex].id,
                            result.bookId, result.chapterNum, hcd ? &hcd->words : nullptr);
                    }
                }
                uiManager_->HideRadialMenu();
            }
            return;
        }
        uiManager_->HideRadialMenu();
        accumStartWord_ = -1;
        accumEndWord_ = -1;
    }
}

void App::OnDragStart(int startWord, Vector2 /*pos*/) {
    auto& ph = inputHandler_->GetPressStartHit();
    eventBus_->Emit(theword::event::SelectionEvent{
        theword::event::SelectionEvent::Action::Start, startWord, startWord, ph.bookId, ph.chapterNum});
}

void App::OnDragUpdate(int startWord, int currentWord, Vector2 /*pos*/) {
    auto& ph = inputHandler_->GetPressStartHit();
    eventBus_->Emit(theword::event::SelectionEvent{
        theword::event::SelectionEvent::Action::Update, startWord, currentWord, ph.bookId, ph.chapterNum});
}

void App::OnDragEnd(int startWord, int endWord, Vector2 pos) {
    if (longPressHandled_) {
        longPressHandled_ = false;
        return;
    }
    auto& ph = inputHandler_->GetPressStartHit();
    eventBus_->Emit(theword::event::SelectionEvent{
        theword::event::SelectionEvent::Action::End, startWord, endWord, ph.bookId, ph.chapterNum});
    uiManager_->ShowRadialMenu(pos, startWord, endWord, ph.bookId, ph.chapterNum);
}

void App::OnLongPress(int wordId, Vector2 pos) {
    auto& ph = inputHandler_->GetPressStartHit();
    eventBus_->Emit(theword::event::SelectionEvent{
        theword::event::SelectionEvent::Action::Start, wordId, wordId, ph.bookId, ph.chapterNum});
    if (!platform::HasTouchInput() && highlighter_->IsWordHighlighted(wordId, ph.bookId, ph.chapterNum)) {
        eventBus_->Emit(theword::event::SelectionEvent{
            theword::event::SelectionEvent::Action::End, wordId, wordId, ph.bookId, ph.chapterNum});
        uiManager_->ShowRadialMenu(pos, wordId, wordId, ph.bookId, ph.chapterNum);
        longPressHandled_ = true;
    } else {
        longPressHandled_ = false;
    }
}

bool App::OnDismiss() {
    if (uiManager_->IsFootnotePopupActive()) {
        uiManager_->HideFootnotePopup();
        return true;
    }
    if (uiManager_->IsRadialMenuActive()) {
        uiManager_->HideRadialMenu();
        accumStartWord_ = -1;
        accumEndWord_ = -1;
        return true;
    }
    return false;
}

void App::CopySelection(const ChapterData& data, int startWord, int endWord) {
    std::string text = AssembleSelectedText(data, startWord, endWord);
    if (!text.empty()) {
        platform::SetClipboard(text);
        Logger::Debug("Copied to clipboard: " + text);
        if (uiManager_) uiManager_->ShowToast("Copiado!");
    }
}

void App::Run() {
    WireInputCallbacks();
    MainLoop();
}

} // namespace theword::app
