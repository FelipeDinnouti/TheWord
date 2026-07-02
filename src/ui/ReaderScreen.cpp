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
                           const theword::core::UIScale& uiScale,
                           float& currentFontSize, bool& versionOnline)
    : eventBus_(eventBus)
    , docManager_(docManager)
    , renderer_(renderer)
    , highlighter_(highlighter)
    , persistence_(persistence)
    , uiFont_(uiFont)
    , uiFontSize_(uiFontSize)
    , contentTop_(contentTop)
    , navStack_(navStack)
    , uiScale_(uiScale)
    , currentFontSize_(currentFontSize)
    , versionOnline_(versionOnline)
    , bottomBarHeight_(std::max(uiScale_.dp(48), uiFontSize * 0.7f + uiScale_.dp(16)))
    , bottomMargin_(uiScale_.bottomInset) {

    barAnimation_ = bottomBarHeight_;

    eventBus_.On<ScrollEvent>([this](const ScrollEvent& e) { OnScroll(e); });
    eventBus_.On<NavigateToHighlightEvent>([this](const auto& e) { OnNavigateToHighlight(e); });
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
    float target = showBottomBar_ ? bottomBarHeight_ : 0.0f;
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

    // Selection tint during drag
    if (highlighter_.IsSelecting()) {
        int selStart = highlighter_.GetSelectionStart();
        int selEnd = highlighter_.GetSelectionEnd();
        if (selStart >= 0 && selEnd >= 0) {
            int minW = std::min(selStart, selEnd);
            int maxW = std::max(selStart, selEnd);
            for (const auto& [span, docY] : docSpans) {
                if (span.startWord >= 0 && span.endWord >= minW && span.startWord <= maxW) {
                    float screenY = docY - scrollY + contentTop_;
                    DrawRectangle(static_cast<int>(span.x),
                                  static_cast<int>(screenY),
                                  static_cast<int>(span.width),
                                  static_cast<int>(span.height),
                                  {135, 206, 250, 80});
                }
            }
        }
    }

    // Handle pending navigation from Highlight Browser tap
    if (pendingNavigateWordId_ >= 0) {
        float targetY = FindLineYForWord(pendingNavigateWordId_);
        if (targetY >= 0.0f) {
            docManager_.ScrollTo(targetY - contentTop_);
            pendingNavigateWordId_ = -1;
        }
    }

    float deltaTime = GetFrameTime();
    UpdateBottomBar(deltaTime);

    DrawBottomBarContent();

    // Cursor: hand over bottom bar, I-beam only during active selection
    Vector2 mouse = GetMousePosition();
    float screenH = static_cast<float>(GetScreenHeight());
    float bottomTop = screenH - barAnimation_ - bottomMargin_;
    if (mouse.y > bottomTop) {
        SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
    } else if (highlighter_.IsSelecting()) {
        SetMouseCursor(MOUSE_CURSOR_IBEAM);
    } else {
        SetMouseCursor(MOUSE_CURSOR_DEFAULT);
    }
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
    float barY = screenH - barAnimation_ - bottomMargin_;

    if (mousePos.y < barY) return false;

    float hitArea = uiScale_.dp(56);
    if (mousePos.x < hitArea) {
        std::string ref = docManager_.GetCurrentChapterId();
        std::string prev = GetPreviousChapter(ref);
        if (!prev.empty()) {
            eventBus_.Emit(NavigateEvent{prev});
        }
        return true;
    }

    if (mousePos.x >= static_cast<float>(GetScreenWidth()) - hitArea) {
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
        uiScale_, currentFontSize_, versionOnline_,
        docManager_.GetCurrentChapterId()
    ));
}

void ReaderScreen::OnNavigateToHighlight(const theword::event::NavigateToHighlightEvent& e) {
    pendingNavigateWordId_ = e.wordId;
    docManager_.LoadInitialChapter(e.chapterRef);
}

float ReaderScreen::FindLineYForWord(int wordId) const {
    auto chapterLayout = docManager_.GetCurrentLayout();
    if (!chapterLayout) return -1.0f;
    for (const auto& line : chapterLayout->lines) {
        for (const auto& span : line.spans) {
            if (span.startWord >= 0 && wordId >= span.startWord && wordId <= span.endWord) {
                return line.y;
            }
        }
    }
    return -1.0f;
}

void ReaderScreen::DrawBottomBarContent() {
    float screenW = static_cast<float>(GetScreenWidth());
    float screenH = static_cast<float>(GetScreenHeight());

    float bottomHeight = barAnimation_ + bottomMargin_;
    float bottomTop = screenH - bottomHeight;
    DrawRectangle(0, static_cast<int>(bottomTop), static_cast<int>(screenW),
                  static_cast<int>(bottomHeight), theme::WINDOW_BG);

    if (barAnimation_ < 1.0f) return;

    DrawRectangle(0, static_cast<int>(bottomTop), static_cast<int>(screenW), 1, LIGHTGRAY);

    std::string ref = docManager_.GetCurrentChapterId();
    std::string book;
    int chapter = 1;
    ParseChapterRef(ref, book, chapter);
    std::string chapterRef = std::string(book) + " " + std::to_string(chapter);

    float fontSize = uiFontSize_ * 0.7f;
    Vector2 textSize = MeasureTextEx(uiFont_, chapterRef.c_str(), fontSize, 1);
    float centerX = (screenW - textSize.x) / 2.0f;
    float centerY = bottomTop + (barAnimation_ - textSize.y) / 2.0f;

    float fadeRatio = barAnimation_ / bottomBarHeight_;
    float alpha = std::clamp((fadeRatio - 0.4f) / 0.6f, 0.0f, 1.0f);
    Color textColor = Fade(theme::UI_TEXT, alpha);

    Vector2 mouse = GetMousePosition();
    float hitArea = uiScale_.dp(56);
    bool overLeft = mouse.y > bottomTop && mouse.x < hitArea;
    bool overRight = mouse.y > bottomTop && mouse.x >= screenW - hitArea;

    Color arrowColorLeft = textColor;
    Color arrowColorRight = textColor;

    if (overLeft) {
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            arrowColorLeft = Fade(theme::UI_TEXT, alpha * 0.3f);
        } else {
            arrowColorLeft = Fade(theme::UI_TITLE, alpha);
        }
    }
    if (overRight) {
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            arrowColorRight = Fade(theme::UI_TEXT, alpha * 0.3f);
        } else {
            arrowColorRight = Fade(theme::UI_TITLE, alpha);
        }
    }

    DrawTextEx(uiFont_, "<", {uiScale_.dp(12), centerY + 1.0f}, fontSize, 1, arrowColorLeft);
    DrawTextEx(uiFont_, chapterRef.c_str(), {centerX, centerY}, fontSize, 1, textColor);
    Vector2 arrowSize = MeasureTextEx(uiFont_, ">", fontSize, 1);
    DrawTextEx(uiFont_, ">", {screenW - uiScale_.dp(12) - arrowSize.x, centerY + 1.0f}, fontSize, 1, arrowColorRight);
}

} // namespace theword::ui
