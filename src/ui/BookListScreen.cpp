#include "BookListScreen.h"
#include "ChapterGridScreen.h"
#include "NavigationStack.h"
#include "components.h"
#include "core/BibleBooks.h"
#include "core/Theme.h"
#include "core/Config.h"
#include "event/EventBus.h"
#include <algorithm>

namespace theword::ui {

using namespace theword::core;

BookListScreen::BookListScreen(const Font& font, float fontSize,
                               NavigationStack& navStack,
                               theword::event::EventBus& eventBus)
    : font_(font), fontSize_(fontSize),
      navStack_(navStack), eventBus_(eventBus) {}

std::vector<int> BookListScreen::GetFilteredIndices() const {
    if (search_.empty()) {
        std::vector<int> all(BOOKS.size());
        for (size_t i = 0; i < BOOKS.size(); ++i) all[i] = static_cast<int>(i);
        return all;
    }
    std::vector<int> results;
    for (int i = 0; i < static_cast<int>(BOOKS.size()); ++i) {
        if (StartsWithIgnoreCase(BOOKS[i].code, search_) ||
            StartsWithIgnoreCase(BOOKS[i].fullName, search_)) {
            results.push_back(i);
        }
    }
    return results;
}

int BookListScreen::GetVisibleCount(float listHeight) const {
    return static_cast<int>(listHeight / ITEM_HEIGHT);
}

void BookListScreen::Draw() {
    float screenW = static_cast<float>(GetScreenWidth());
    float screenH = static_cast<float>(GetScreenHeight());

    DrawHeaderBar(font_, fontSize_, "Books", true, static_cast<int>(screenW));

    // Search bar
    float searchY = HEADER_HEIGHT + 4.0f;
    Rectangle searchBox = {12.0f, searchY, screenW - 24.0f, SEARCH_HEIGHT - 8.0f};
    DrawRectangleRec(searchBox, theme::INPUT_BG);
    DrawRectangleLinesEx(searchBox, 1, theme::INPUT_BORDER);

    std::string display = search_;
    display += "|";
    float labelSize = fontSize_ * 0.65f;
    DrawTextEx(font_, display.c_str(), {searchBox.x + 6.0f, searchBox.y + 4.0f},
               labelSize, 1, theme::UI_INPUT_TEXT);

    // Book list
    float listY = HEADER_HEIGHT + SEARCH_HEIGHT + 4.0f;
    float listH = screenH - listY;

    auto indices = GetFilteredIndices();
    int visibleCount = GetVisibleCount(listH);
    int maxScroll = std::max(0, static_cast<int>(indices.size()) - visibleCount);
    scrollOffset_ = std::min(scrollOffset_, maxScroll);

    for (int i = scrollOffset_; i < static_cast<int>(indices.size()); ++i) {
        int itemIdx = i - scrollOffset_;
        float itemY = listY + itemIdx * ITEM_HEIGHT;
        if (itemY + ITEM_HEIGHT > screenH) break;

        int bookIdx = indices[i];
        std::string label = std::string(BOOKS[bookIdx].code) + "  " + BOOKS[bookIdx].fullName;

        bool selected = (bookIdx == selection_);
        if (selected) {
            DrawRectangle(0, static_cast<int>(itemY), static_cast<int>(screenW),
                          static_cast<int>(ITEM_HEIGHT), theme::SELECTED_BG);
        }

        DrawTextEx(font_, label.c_str(), {20.0f, itemY + 8.0f},
                   labelSize, 1, selected ? theme::UI_TITLE : theme::UI_TEXT);
    }
}

bool BookListScreen::HandleInput(float /*deltaTime*/) {
    int ch = GetCharPressed();
    while (ch > 0) {
        if (ch >= 32 && ch <= 126) {
            search_.push_back(static_cast<char>(ch));
            scrollOffset_ = 0;
            selection_ = 0;
        }
        ch = GetCharPressed();
    }

    if (IsKeyPressed(key::BACKSPACE)) {
        if (!search_.empty()) {
            search_.pop_back();
            scrollOffset_ = 0;
            selection_ = 0;
        } else {
            navStack_.Pop();
            return true;
        }
    }

    if (IsKeyPressed(key::ESCAPE)) {
        navStack_.Pop();
        return true;
    }

    // Scroll wheel
    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) {
        auto indices = GetFilteredIndices();
        float listY = HEADER_HEIGHT + SEARCH_HEIGHT + 4.0f;
        float screenH = static_cast<float>(GetScreenHeight());
        float listH = screenH - listY;
        int visibleCount = static_cast<int>(listH / ITEM_HEIGHT);
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

        // Auto-scroll to keep selection visible
        float listY = HEADER_HEIGHT + SEARCH_HEIGHT + 4.0f;
        float screenH = static_cast<float>(GetScreenHeight());
        float listH = screenH - listY;
        int visibleCount = static_cast<int>(listH / ITEM_HEIGHT);
        if (newIdx < scrollOffset_) scrollOffset_ = newIdx;
        if (newIdx >= scrollOffset_ + visibleCount) scrollOffset_ = newIdx - visibleCount + 1;
        return true;
    }

    if (IsKeyPressed(key::ENTER)) {
        auto indices = GetFilteredIndices();
        for (int i = 0; i < static_cast<int>(indices.size()); ++i) {
            if (indices[i] == selection_) {
                const auto& book = BOOKS[selection_];
                navStack_.Push(std::make_unique<ChapterGridScreen>(
                    font_, fontSize_, navStack_, eventBus_,
                    book.code, book.fullName, book.chapterCount
                ));
                return true;
            }
        }
    }

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 mousePos = GetMousePosition();
        // Back button area
        if (mousePos.y < HEADER_HEIGHT && mousePos.x < BACK_AREA_WIDTH) {
            navStack_.Pop();
            return true;
        }

        // Search bar click — focus (already focused for keyboard input)
        // Book list click
        float listY = HEADER_HEIGHT + SEARCH_HEIGHT + 4.0f;
        if (mousePos.y >= listY) {
            int itemIdx = static_cast<int>((mousePos.y - listY) / ITEM_HEIGHT);
            auto indices = GetFilteredIndices();
            int bookIdx = scrollOffset_ + itemIdx;
            if (bookIdx >= 0 && bookIdx < static_cast<int>(indices.size())) {
                selection_ = indices[bookIdx];
                const auto& book = BOOKS[selection_];
                navStack_.Push(std::make_unique<ChapterGridScreen>(
                    font_, fontSize_, navStack_, eventBus_,
                    book.code, book.fullName, book.chapterCount
                ));
                return true;
            }
        }
    }

    return false;
}

} // namespace theword::ui
