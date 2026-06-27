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
                                     int chapterCount)
    : font_(font), fontSize_(fontSize),
      navStack_(navStack), eventBus_(eventBus),
      bookCode_(bookCode), bookName_(bookName),
      chapterCount_(chapterCount) {}

void ChapterGridScreen::Draw() {
    float screenW = static_cast<float>(GetScreenWidth());

    DrawHeaderBar(font_, fontSize_, bookName_.c_str(), true, static_cast<int>(screenW));

    float gridY = HEADER_HEIGHT + GRID_PADDING;
    float totalRowWidth = GRID_COLUMNS * CELL_WIDTH + (GRID_COLUMNS - 1) * CELL_GAP;
    float gridStartX = (screenW - totalRowWidth) / 2.0f;

    float labelSize = fontSize_ * 0.6f;

    for (int ch = 1; ch <= chapterCount_; ++ch) {
        int col = (ch - 1) % GRID_COLUMNS;
        int row = (ch - 1) / GRID_COLUMNS;

        float cx = gridStartX + col * (CELL_WIDTH + CELL_GAP);
        float cy = gridY + row * (CELL_HEIGHT + CELL_GAP);

        // Cell background
        Color cellBg = (ch == selectedChapter_) ? theme::SELECTED_BG : theme::BUTTON_BG;
        DrawRectangle(static_cast<int>(cx), static_cast<int>(cy),
                      static_cast<int>(CELL_WIDTH), static_cast<int>(CELL_HEIGHT),
                      cellBg);
        DrawRectangleLines(static_cast<int>(cx), static_cast<int>(cy),
                           static_cast<int>(CELL_WIDTH), static_cast<int>(CELL_HEIGHT),
                           theme::BUTTON_BORDER);

        // Number
        std::string num = std::to_string(ch);
        Vector2 numSize = MeasureTextEx(font_, num.c_str(), labelSize, 1);
        float numX = cx + (CELL_WIDTH - numSize.x) / 2.0f;
        float numY = cy + (CELL_HEIGHT - numSize.y) / 2.0f;
        DrawTextEx(font_, num.c_str(), {numX, numY}, labelSize, 1, theme::UI_TEXT);
    }
}

bool ChapterGridScreen::HandleInput(float /*deltaTime*/) {
    if (IsKeyPressed(key::ESCAPE)) {
        navStack_.Pop();
        return true;
    }

    // Keyboard grid navigation
    if (IsKeyPressed(key::LEFT) && selectedChapter_ > 1) {
        selectedChapter_--;
        return true;
    }
    if (IsKeyPressed(key::RIGHT) && selectedChapter_ < chapterCount_) {
        selectedChapter_++;
        return true;
    }
    if (IsKeyPressed(key::UP)) {
        int target = selectedChapter_ - GRID_COLUMNS;
        if (target >= 1) {
            selectedChapter_ = target;
        }
        return true;
    }
    if (IsKeyPressed(key::DOWN)) {
        int target = selectedChapter_ + GRID_COLUMNS;
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

        // Back button
        if (mousePos.y < HEADER_HEIGHT && mousePos.x < BACK_AREA_WIDTH) {
            navStack_.Pop();
            return true;
        }

        // Grid click
        float gridY = HEADER_HEIGHT + GRID_PADDING;
        float totalRowWidth = GRID_COLUMNS * CELL_WIDTH + (GRID_COLUMNS - 1) * CELL_GAP;
        float gridStartX = (screenW - totalRowWidth) / 2.0f;

        if (mousePos.y >= gridY && mousePos.x >= gridStartX &&
            mousePos.x < gridStartX + totalRowWidth) {
            int col = static_cast<int>((mousePos.x - gridStartX) / (CELL_WIDTH + CELL_GAP));
            int row = static_cast<int>((mousePos.y - gridY) / (CELL_HEIGHT + CELL_GAP));
            int chapter = row * GRID_COLUMNS + col + 1;

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
