#include "ChapterGridScreen.h"
#include "NavigationStack.h"
#include "components.h"
#include "core/Theme.h"
#include "core/Config.h"
#include "core/BibleBooks.h"
#include "event/EventBus.h"
#include "event/Events.h"

namespace theword::ui {

using namespace theword::core;

ChapterGridScreen::ChapterGridScreen(const Font& font, float fontSize,
                                     NavigationStack& navStack,
                                     theword::event::EventBus& eventBus,
                                     const std::string& bookCode,
                                     const std::string& bookName,
                                     int chapterCount,
                                     const theword::core::UIScale& uiScale)
    : font_(font), fontSize_(fontSize),
      navStack_(navStack), eventBus_(eventBus),
      bookCode_(bookCode), bookName_(bookName),
      chapterCount_(chapterCount), uiScale_(uiScale) {}

void ChapterGridScreen::Draw() {
    float screenW = static_cast<float>(GetScreenWidth());

    float headerH = uiScale_.dp(48);
    float padding = uiScale_.dp(12);
    float cellW = uiScale_.dp(56);
    float cellH = uiScale_.dp(48);
    float gap = uiScale_.dp(8);
    int columns = 5;

    DrawHeaderBar(font_, fontSize_, bookName_.c_str(), true, static_cast<int>(screenW), uiScale_);

    float gridY = headerH + padding;
    float totalRowWidth = columns * cellW + (columns - 1) * gap;
    float gridStartX = (screenW - totalRowWidth) / 2.0f;

    float labelSize = fontSize_ * 0.6f;

    for (int ch = 1; ch <= chapterCount_; ++ch) {
        int col = (ch - 1) % columns;
        int row = (ch - 1) / columns;

        float cx = gridStartX + col * (cellW + gap);
        float cy = gridY + row * (cellH + gap);

        Color cellBg = (ch == selectedChapter_) ? theme::SELECTED_BG : theme::BUTTON_BG;
        DrawRectangle(static_cast<int>(cx), static_cast<int>(cy),
                      static_cast<int>(cellW), static_cast<int>(cellH),
                      cellBg);
        DrawRectangleLines(static_cast<int>(cx), static_cast<int>(cy),
                           static_cast<int>(cellW), static_cast<int>(cellH),
                           theme::BUTTON_BORDER);

        std::string num = std::to_string(ch);
        Vector2 numSize = MeasureTextEx(font_, num.c_str(), labelSize, 1);
        float numX = cx + (cellW - numSize.x) / 2.0f;
        float numY = cy + (cellH - numSize.y) / 2.0f;
        DrawTextEx(font_, num.c_str(), {numX, numY}, labelSize, 1, theme::UI_TEXT);
    }
}

bool ChapterGridScreen::HandleInput(float /*deltaTime*/) {
    if (IsKeyPressed(key::ESCAPE)) {
        navStack_.Pop();
        return true;
    }

    float headerH = uiScale_.dp(48);
    float padding = uiScale_.dp(12);
    float cellW = uiScale_.dp(56);
    float cellH = uiScale_.dp(48);
    float gap = uiScale_.dp(8);
    int columns = 5;

    if (IsKeyPressed(key::LEFT) && selectedChapter_ > 1) {
        selectedChapter_--;
        return true;
    }
    if (IsKeyPressed(key::RIGHT) && selectedChapter_ < chapterCount_) {
        selectedChapter_++;
        return true;
    }
    if (IsKeyPressed(key::UP)) {
        int target = selectedChapter_ - columns;
        if (target >= 1) {
            selectedChapter_ = target;
        }
        return true;
    }
    if (IsKeyPressed(key::DOWN)) {
        int target = selectedChapter_ + columns;
        if (target <= chapterCount_) {
            selectedChapter_ = target;
        }
        return true;
    }

    if (IsKeyPressed(key::ENTER)) {
        std::string ref = bookCode_ + "." + std::to_string(selectedChapter_);
        auto& bus = eventBus_;
        navStack_.PopAll();
        bus.Emit(theword::event::NavigateEvent{ref});
        return true;
    }

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 mousePos = GetMousePosition();
        float screenW = static_cast<float>(GetScreenWidth());
        float backW = uiScale_.dp(56);

        // Back button
        if (mousePos.y < headerH && mousePos.x < backW) {
            navStack_.Pop();
            return true;
        }

        // Grid click
        float gridY = headerH + padding;
        float totalRowWidth = columns * cellW + (columns - 1) * gap;
        float gridStartX = (screenW - totalRowWidth) / 2.0f;

        if (mousePos.y >= gridY && mousePos.x >= gridStartX &&
            mousePos.x < gridStartX + totalRowWidth) {
            int col = static_cast<int>((mousePos.x - gridStartX) / (cellW + gap));
            int row = static_cast<int>((mousePos.y - gridY) / (cellH + gap));
            int chapter = row * columns + col + 1;

            if (chapter >= 1 && chapter <= chapterCount_) {
                std::string ref = bookCode_ + "." + std::to_string(chapter);
                auto& bus = eventBus_;
                navStack_.PopAll();
                bus.Emit(theword::event::NavigateEvent{ref});
                return true;
            }
        }
    }

    return false;
}

} // namespace theword::ui
