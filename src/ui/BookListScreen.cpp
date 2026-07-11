#include "BookListScreen.h"
#include "ChapterGridScreen.h"
#include "NavigationStack.h"
#include "components.h"
#include "core/BibleBooks.h"
#include "core/Theme.h"
#include "core/Config.h"
#include "core/Platform.h"
#include "core/Locale.h"
#include "event/EventBus.h"
#include "event/Events.h"
#include <algorithm>

namespace theword::ui {

using namespace theword::core;

BookListScreen::BookListScreen(const Font& font, float fontSize,
                               NavigationStack& navStack,
                               theword::event::EventBus& eventBus,
                               const theword::core::UIScale& uiScale,
                               const std::string& currentChapterRef)
    : font_(font), fontSize_(fontSize),
      navStack_(navStack), eventBus_(eventBus), uiScale_(uiScale),
      currentChapterRef_(currentChapterRef),
      tapDetector_(uiScale_.dp(10)) {
    if (!currentChapterRef_.empty()) {
        std::string bookCode;
        int chapter = 1;
        if (ParseChapterRef(currentChapterRef_, bookCode, chapter)) {
            for (int i = 0; i < static_cast<int>(BOOKS.size()); ++i) {
                if (BOOKS[i].code == bookCode) {
                    selection_ = i;
                    break;
                }
            }
        }
    }

    eventBus_.On<theword::event::ScrollEvent>(
        [this, alive = aliveGuard_](const theword::event::ScrollEvent& e) {
        if (!*alive) return;
        float itemH = std::max(uiScale_.dp(44), fontSize_ * 0.65f + uiScale_.dp(10));
        scrollAccumulator_ += e.delta;
        int itemDelta = static_cast<int>(scrollAccumulator_ / itemH);
        if (itemDelta == 0) return;
        scrollAccumulator_ -= itemDelta * itemH;
        auto indices = GetFilteredIndices();
        float listY = uiScale_.dp(48) + uiScale_.dp(44) + uiScale_.dp(4);
        float screenH = static_cast<float>(GetScreenHeight());
        float listH = screenH - listY;
        int visibleCount = static_cast<int>(listH / itemH);
        int maxScroll = std::max(0, static_cast<int>(indices.size()) - visibleCount);
        scrollOffset_ = std::clamp(scrollOffset_ + itemDelta, 0, maxScroll);
    });
}

BookListScreen::~BookListScreen() {
    *aliveGuard_ = false;
    theword::core::platform::HideKeyboard();
}

std::vector<int> BookListScreen::GetFilteredIndices() const {
    if (search_.empty()) {
        std::vector<int> all(BOOKS.size());
        for (size_t i = 0; i < BOOKS.size(); ++i) all[i] = static_cast<int>(i);
        return all;
    }
    std::vector<int> results;
    for (int i = 0; i < static_cast<int>(BOOKS.size()); ++i) {
        if (StartsWithIgnoreCase(BOOKS[i].code, search_) ||
            StartsWithIgnoreCase(BOOKS[i].fullName, search_) ||
            StartsWithIgnoreCase(BOOK_NAMES_PT[i], search_)) {
            results.push_back(i);
        }
    }
    return results;
}

int BookListScreen::GetVisibleCount(float listHeight) const {
    float itemH = std::max(uiScale_.dp(44), fontSize_ * 0.65f + uiScale_.dp(10));
    return static_cast<int>(listHeight / itemH);
}

void BookListScreen::Draw() {
    float screenW = static_cast<float>(GetScreenWidth());
    float screenH = static_cast<float>(GetScreenHeight());

    float headerH = uiScale_.dp(48);
    float searchH = uiScale_.dp(44);
    float itemH = std::max(uiScale_.dp(44), fontSize_ * 0.65f + uiScale_.dp(10));

    // Scroll to the current book on first draw
    if (!selectionInitialized_ && !currentChapterRef_.empty()) {
        selectionInitialized_ = true;
        float listY = headerH + searchH + uiScale_.dp(4);
        float listH = screenH - listY;
        int visibleCount = static_cast<int>(listH / itemH);
        int maxScroll = std::max(0, static_cast<int>(BOOKS.size()) - visibleCount);
        scrollOffset_ = std::min(maxScroll, std::max(0, selection_ - visibleCount / 2));
    }

    DrawHeaderBar(font_, fontSize_, Locale::Get("Books"), true, static_cast<int>(screenW), uiScale_);

    // Search bar
    float searchY = headerH + uiScale_.dp(4);
    Rectangle searchBox = {uiScale_.dp(12), searchY, screenW - uiScale_.dp(24), searchH - uiScale_.dp(8)};
    DrawRectangleRec(searchBox, theme::INPUT_BG);
    DrawRectangleLinesEx(searchBox, 1, theme::INPUT_BORDER);

    std::string display = search_;
    display += "|";
    float labelSize = fontSize_ * 0.65f;
    DrawTextEx(font_, display.c_str(), {searchBox.x + uiScale_.dp(6), searchBox.y + uiScale_.dp(4)},
               labelSize, 1, theme::UI_INPUT_TEXT);

    // Book list
    float listY = headerH + searchH + uiScale_.dp(4);
    float listH = screenH - listY;

    auto indices = GetFilteredIndices();
    int visibleCount = GetVisibleCount(listH);
    int maxScroll = std::max(0, static_cast<int>(indices.size()) - visibleCount);
    scrollOffset_ = std::min(scrollOffset_, maxScroll);

    for (int i = scrollOffset_; i < static_cast<int>(indices.size()); ++i) {
        int itemIdx = i - scrollOffset_;
        float itemY = listY + itemIdx * itemH;
        if (itemY + itemH > screenH) break;

        int bookIdx = indices[i];
        std::string label = std::string(BOOKS[bookIdx].code) + "  " + BOOK_NAMES_PT[bookIdx];

        DrawTextItem({0, itemY, screenW, itemH}, label.c_str(), font_, labelSize,
                     bookIdx == selection_, theme::UI_TEXT, theme::UI_TITLE);
    }

    Vector2 mouse = GetMousePosition();
    bool overRows = mouse.y >= listY;
    SetMouseCursor(overRows ? MOUSE_CURSOR_POINTING_HAND : MOUSE_CURSOR_DEFAULT);
}

bool BookListScreen::HandleInput(float /*deltaTime*/) {
    int ch = GetCharPressed();
    while (ch > 0) {
#if defined(__ANDROID__)
        if (ch == '\b') {
            if (!search_.empty()) {
                search_.pop_back();
                scrollOffset_ = 0;
                scrollAccumulator_ = 0;
                selection_ = 0;
            } else {
                navStack_.Pop();
                return true;
            }
        } else
#endif
        if (ch >= 32 && ch <= 126) {
            search_.push_back(static_cast<char>(ch));
            scrollOffset_ = 0;
            scrollAccumulator_ = 0;
            selection_ = 0;
        }
        ch = GetCharPressed();
    }

#if !defined(__ANDROID__)
    if (IsKeyPressed(key::BACKSPACE)) {
        if (!search_.empty()) {
            search_.pop_back();
            scrollOffset_ = 0;
            scrollAccumulator_ = 0;
            selection_ = 0;
        } else {
            navStack_.Pop();
            return true;
        }
    }
#endif

    if (IsKeyPressed(key::ESCAPE)) {
        navStack_.Pop();
        return true;
    }

    float headerH = uiScale_.dp(48);
    float searchH = uiScale_.dp(44);
    float itemH = std::max(uiScale_.dp(44), fontSize_ * 0.65f + uiScale_.dp(10));

    // Scroll wheel
    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) {
        auto indices = GetFilteredIndices();
        float listY = headerH + searchH + uiScale_.dp(4);
        float screenH = static_cast<float>(GetScreenHeight());
        float listH = screenH - listY;
        int visibleCount = static_cast<int>(listH / itemH);
        int maxScroll = std::max(0, static_cast<int>(indices.size()) - visibleCount);
        scrollOffset_ = std::clamp(scrollOffset_ - static_cast<int>(wheel), 0, maxScroll);
        return true;
    }

    // Up/down arrow navigation
    if (IsKeyPressed(key::UP) || IsKeyPressed(key::DOWN)) {
        auto indices = GetFilteredIndices();
        if (indices.empty()) return true;

        int step = IsKeyPressed(key::DOWN) ? 1 : -1;
        int curIdx = 0;
        for (int i = 0; i < static_cast<int>(indices.size()); ++i) {
            if (indices[i] == selection_) { curIdx = i; break; }
        }
        int newIdx = std::clamp(curIdx + step, 0, static_cast<int>(indices.size()) - 1);
        selection_ = indices[newIdx];

        float listY = headerH + searchH + uiScale_.dp(4);
        float screenH = static_cast<float>(GetScreenHeight());
        float listH = screenH - listY;
        int visibleCount = static_cast<int>(listH / itemH);
        if (newIdx < scrollOffset_) scrollOffset_ = newIdx;
        if (newIdx >= scrollOffset_ + visibleCount) scrollOffset_ = newIdx - visibleCount + 1;
        return true;
    }

    if (IsKeyPressed(key::ENTER)) {
        auto indices = GetFilteredIndices();
        for (int i = 0; i < static_cast<int>(indices.size()); ++i) {
            if (indices[i] == selection_) {
                const auto& book = BOOKS[selection_];
                int currentCh = 1;
                if (!currentChapterRef_.empty()) {
                    std::string refBook;
                    int refCh = 1;
                    if (ParseChapterRef(currentChapterRef_, refBook, refCh) && refBook == book.code) {
                        currentCh = refCh;
                    }
                }
                theword::core::platform::HideKeyboard();
                navStack_.Push(std::make_unique<ChapterGridScreen>(
                    font_, fontSize_, navStack_, eventBus_,
                    book.code, BOOK_NAMES_PT[selection_], book.chapterCount, uiScale_, currentCh
                ));
                return true;
            }
        }
    }

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        tapDetector_.OnPress(GetMousePosition());

    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        Vector2 mousePos;
        auto tr = tapDetector_.OnRelease(GetMousePosition(), mousePos);
        if (tr == TapDetector::Result::Drag) { return false; }
        if (tr == TapDetector::Result::Tap) {

        float backW = uiScale_.dp(56);

        // Back button area
        if (mousePos.y < headerH && mousePos.x < backW) {
            navStack_.Pop();
            return true;
        }

        // Search bar tap — show keyboard
        if (mousePos.y >= headerH && mousePos.y < headerH + searchH) {
            theword::core::platform::ShowKeyboard();
            return true;
        }

        // Book list click
        float listY = headerH + searchH + uiScale_.dp(4);
        if (mousePos.y >= listY) {
            int itemIdx = static_cast<int>((mousePos.y - listY) / itemH);
            auto indices = GetFilteredIndices();
            int bookIdx = scrollOffset_ + itemIdx;
            if (bookIdx >= 0 && bookIdx < static_cast<int>(indices.size())) {
                selection_ = indices[bookIdx];
                const auto& book = BOOKS[selection_];
                int currentCh = 1;
                if (!currentChapterRef_.empty()) {
                    std::string refBook;
                    int refCh = 1;
                    if (ParseChapterRef(currentChapterRef_, refBook, refCh) && refBook == book.code) {
                        currentCh = refCh;
                    }
                }
                theword::core::platform::HideKeyboard();
                navStack_.Push(std::make_unique<ChapterGridScreen>(
                    font_, fontSize_, navStack_, eventBus_,
                    book.code, BOOK_NAMES_PT[selection_], book.chapterCount, uiScale_, currentCh
                ));
                return true;
            }
        }
    }
    }

    return false;
}

} // namespace theword::ui
