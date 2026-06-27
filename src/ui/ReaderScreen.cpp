#include "ReaderScreen.h"
#include "NavigationStack.h"
#include "CenterMenu.h"
#include "data/ChapterProvider.h"
#include "document/DocumentManager.h"
#include "renderer/Renderer.h"
#include "highlight/Highlighter.h"
#include "persistence/PersistenceManager.h"
#include "core/Theme.h"
#include "core/BibleBooks.h"
#include "core/Config.h"
#include "event/EventBus.h"
#include "event/Events.h"
#include <cmath>

namespace theword::ui {

using namespace theword::data;
using namespace theword::renderer;
using namespace theword::core;
using namespace theword::event;

ReaderScreen::ReaderScreen(theword::event::EventBus& eventBus,
                           theword::document::DocumentManager& docManager,
                           theword::renderer::Renderer& renderer,
                           theword::highlight::Highlighter& highlighter,
                           theword::persistence::PersistenceManager& persistence,
                           const Font& uiFont, float uiFontSize,
                           float contentTop,
                           NavigationStack& navStack,
                           float scale, float& currentFontSize, bool& versionOnline)
    : eventBus_(eventBus)
    , docManager_(docManager)
    , renderer_(renderer)
    , highlighter_(highlighter)
    , persistence_(persistence)
    , uiFont_(uiFont)
    , uiFontSize_(uiFontSize)
    , contentTop_(contentTop)
    , navStack_(navStack)
    , scale_(scale)
    , currentFontSize_(currentFontSize)
    , versionOnline_(versionOnline) {

    barAnimation_ = BOTTOM_BAR_HEIGHT;

    eventBus_.On<ScrollEvent>([this](const ScrollEvent& e) { OnScroll(e); });
}

ReaderScreen::~ReaderScreen() = default;

void ReaderScreen::OnScroll(const ScrollEvent& e) {
    float delta = e.delta;
    if (std::abs(delta) > 0.5f) {
        scrollAccumulator_ += delta;
        float threshold = SHOW_HIDE_THRESHOLD;
        if (scrollAccumulator_ < -threshold) {
            showBottomBar_ = true;
            scrollAccumulator_ = 0.0f;
        } else if (scrollAccumulator_ > threshold) {
            showBottomBar_ = false;
            scrollAccumulator_ = 0.0f;
        }
    }
}

void ReaderScreen::UpdateBottomBar(float deltaTime) {
    float target = showBottomBar_ ? BOTTOM_BAR_HEIGHT : 0.0f;
    float diff = target - barAnimation_;
    if (std::abs(diff) > 0.5f) {
        barAnimation_ += diff * (1.0f - std::exp(-ANIMATION_SPEED * deltaTime));
    } else {
        barAnimation_ = target;
    }
}

void ReaderScreen::Draw() {
    float scrollY = docManager_.GetScrollY();
    float totalHeight = docManager_.GetTotalHeight();
    float viewHeight = docManager_.GetViewportHeight();

    std::vector<std::pair<Span, float>> docSpans;
    docManager_.GetVisibleSpans(docSpans);

    std::vector<HighlightRect> hlRects;
    for (const auto& [span, docY] : docSpans) {
        if (span.startWord >= 0 && highlighter_.IsWordHighlighted(span.startWord)) {
            float screenY = docY - scrollY + contentTop_;
            hlRects.push_back({
                span.x, screenY, span.width, span.height,
                highlighter_.GetHighlightForWord(span.startWord)
            });
        }
    }

    renderer_.DrawFrame(scrollY, totalHeight, viewHeight, docSpans, hlRects);

    float deltaTime = GetFrameTime();
    UpdateBottomBar(deltaTime);

    DrawBottomBarContent();
}

bool ReaderScreen::HandleInput(float /*deltaTime*/) {
    if (HandleBottomBarClick()) return true;

    if (IsKeyPressed(key::G)) {
        OpenCenterMenu();
        return true;
    }

    // Arrow key chapter navigation
    if (IsKeyPressed(key::LEFT)) {
        std::string ref = docManager_.GetCurrentChapterId();
        std::string prev = GetPreviousChapter(ref);
        if (!prev.empty()) {
            eventBus_.Emit(NavigateEvent{prev});
        }
        return true;
    }
    if (IsKeyPressed(key::RIGHT)) {
        std::string ref = docManager_.GetCurrentChapterId();
        std::string next = GetNextChapter(ref);
        if (!next.empty()) {
            eventBus_.Emit(NavigateEvent{next});
        }
        return true;
    }

    return false;
}

bool ReaderScreen::HandleBottomBarClick() {
    if (barAnimation_ < 1.0f) return false;
    if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) return false;

    Vector2 mousePos = GetMousePosition();
    float screenH = static_cast<float>(GetScreenHeight());
    float barY = screenH - barAnimation_;

    if (mousePos.y < barY) return false;

    if (mousePos.x < 50.0f) {
        std::string ref = docManager_.GetCurrentChapterId();
        std::string prev = GetPreviousChapter(ref);
        if (!prev.empty()) {
            eventBus_.Emit(NavigateEvent{prev});
        }
        return true;
    }

    if (mousePos.x >= static_cast<float>(GetScreenWidth()) - 50.0f) {
        std::string ref = docManager_.GetCurrentChapterId();
        std::string next = GetNextChapter(ref);
        if (!next.empty()) {
            eventBus_.Emit(NavigateEvent{next});
        }
        return true;
    }

    OpenCenterMenu();
    return true;
}

void ReaderScreen::OpenCenterMenu() {
    navStack_.Push(std::make_unique<CenterMenu>(
        uiFont_, uiFontSize_, navStack_, eventBus_,
        highlighter_, persistence_,
        scale_, currentFontSize_, versionOnline_
    ));
}

void ReaderScreen::DrawBottomBarContent() {
    if (barAnimation_ < 1.0f) return;

    float screenW = static_cast<float>(GetScreenWidth());
    float screenH = static_cast<float>(GetScreenHeight());
    float barY = screenH - barAnimation_;
    float barH = barAnimation_;

    DrawRectangle(0, static_cast<int>(barY), static_cast<int>(screenW),
                  static_cast<int>(barH), theme::WINDOW_BG);

    DrawRectangle(0, static_cast<int>(barY), static_cast<int>(screenW), 1, LIGHTGRAY);

    std::string ref = docManager_.GetCurrentChapterId();
    std::string book;
    int chapter = 1;
    ParseChapterRef(ref, book, chapter);
    std::string chapterRef = std::string(book) + " " + std::to_string(chapter);

    float fontSize = uiFontSize_ * 0.7f;
    Vector2 textSize = MeasureTextEx(uiFont_, chapterRef.c_str(), fontSize, 1);
    float centerX = (screenW - textSize.x) / 2.0f;
    float centerY = barY + (barH - textSize.y) / 2.0f;
    DrawTextEx(uiFont_, chapterRef.c_str(), {centerX, centerY}, fontSize, 1, theme::UI_TEXT);

    DrawTextEx(uiFont_, "<", {15.0f, centerY + 1.0f}, fontSize, 1, theme::UI_TEXT);
    DrawTextEx(uiFont_, ">", {screenW - 25.0f, centerY + 1.0f}, fontSize, 1, theme::UI_TEXT);
}

} // namespace theword::ui
