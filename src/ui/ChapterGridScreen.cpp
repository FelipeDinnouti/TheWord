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
                                     const theword::core::UIScale& uiScale,
                                     int currentChapter)
    : font_(font), fontSize_(fontSize),
      navStack_(navStack), eventBus_(eventBus),
      bookCode_(bookCode), bookName_(bookName),
      chapterCount_(chapterCount), uiScale_(uiScale),
      tapDetector_(uiScale_.dp(10)) {
    if (currentChapter >= 1 && currentChapter <= chapterCount) {
        selectedChapter_ = currentChapter;
    }

    eventBus_.On<theword::event::ScrollEvent>(
        [this, alive = aliveGuard_](const theword::event::ScrollEvent& e) {
        if (!*alive) return;
        float screenH = static_cast<float>(GetScreenHeight());
        float headerH = uiScale_.dp(48);
        float padding = uiScale_.dp(12);
        float cellH = uiScale_.dp(48);
        float gap = uiScale_.dp(8);
        int columns = 5;
        float gridY = headerH + padding;
        float availableH = screenH - gridY - padding;
        float totalRowH = cellH + gap;
        int totalRows = (chapterCount_ + columns - 1) / columns;
        float totalGridH = totalRows * totalRowH;
        float maxScroll = std::max(0.0f, totalGridH - availableH);
        gridScrollY_ = std::clamp(gridScrollY_ + e.delta, 0.0f, maxScroll);
    });
}

ChapterGridScreen::~ChapterGridScreen() {
    *aliveGuard_ = false;
}

void ChapterGridScreen::Draw() {
    float screenW = static_cast<float>(GetScreenWidth());
    float screenH = static_cast<float>(GetScreenHeight());

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

    int totalRows = (chapterCount_ + columns - 1) / columns;
    float availableH = screenH - gridY - padding;
    float totalGridH = totalRows * (cellH + gap);
    float maxScroll = std::max(0.0f, totalGridH - availableH);
    gridScrollY_ = std::clamp(gridScrollY_, 0.0f, maxScroll);

    float labelSize = fontSize_ * 0.6f;

    // Clip grid content below the header bar
    BeginScissorMode(0, static_cast<int>(headerH), static_cast<int>(screenW),
                     static_cast<int>(screenH - headerH));

    float scrollOffset = gridScrollY_;

    for (int ch = 1; ch <= chapterCount_; ++ch) {
        int col = (ch - 1) % columns;
        int row = (ch - 1) / columns;

        float cx = gridStartX + col * (cellW + gap);
        float cy = gridY + row * (cellH + gap) - scrollOffset;

        // Skip cells outside the visible area
        if (cy + cellH < 0 || cy > screenH) continue;

        std::string num = std::to_string(ch);
        Vector2 numSize = MeasureTextEx(font_, num.c_str(), labelSize, 1);
        float numX = cx + (cellW - numSize.x) / 2.0f;
        float numY = cy + (cellH - numSize.y) / 2.0f;

        if (ch == selectedChapter_) {
            Color bg = theme::ACCENT_TEAL;
            bg.a = 30;
            DrawRectangleRounded({cx, cy, cellW, cellH}, 0.15f, 8, bg);
            DrawRectangleRoundedLines({cx, cy, cellW, cellH}, 0.15f, 8, 1.5f, theme::ACCENT_TEAL);
            DrawTextEx(font_, num.c_str(), {numX, numY}, labelSize, 1, theme::ACCENT_TEAL);
        } else {
            DrawTextEx(font_, num.c_str(), {numX, numY}, labelSize, 1, theme::UI_TEXT);
        }
    }

    // Subtle scrollbar if content overflows
    if (maxScroll > 0.0f) {
        float barW = 4.0f;
        float barH = availableH * (availableH / totalGridH);
        float barX = screenW - barW - 2.0f;
        float barY = gridY + (gridScrollY_ / maxScroll) * (availableH - barH);
        DrawRectangle(static_cast<int>(barX), static_cast<int>(barY),
                      static_cast<int>(barW), static_cast<int>(barH), {128, 128, 128, 120});
    }

    EndScissorMode();

    Vector2 mouse = GetMousePosition();
    bool overGrid = mouse.y >= gridY && mouse.y < gridY + availableH
                    && mouse.x >= gridStartX && mouse.x < gridStartX + totalRowWidth;
    SetMouseCursor(overGrid ? MOUSE_CURSOR_POINTING_HAND : MOUSE_CURSOR_DEFAULT);
}

bool ChapterGridScreen::HandleInput(float /*deltaTime*/) {
    if (IsKeyPressed(key::ESCAPE)) {
        navStack_.Pop();
        return true;
    }

    float screenW = static_cast<float>(GetScreenWidth());
    float screenH = static_cast<float>(GetScreenHeight());
    float headerH = uiScale_.dp(48);
    float padding = uiScale_.dp(12);
    float cellW = uiScale_.dp(56);
    float cellH = uiScale_.dp(48);
    float gap = uiScale_.dp(8);
    int columns = 5;
    float gridY = headerH + padding;
    float availableH = screenH - gridY - padding;
    float totalRowH = cellH + gap;
    int totalRows = (chapterCount_ + columns - 1) / columns;
    float totalGridH = totalRows * totalRowH;
    float maxScroll = std::max(0.0f, totalGridH - availableH);

    // Scroll wheel
    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) {
        gridScrollY_ = std::clamp(gridScrollY_ - wheel * totalRowH * 2.0f, 0.0f, maxScroll);
        return true;
    }

    if (IsKeyPressed(key::LEFT) && selectedChapter_ > 1) {
        selectedChapter_--;
        KeepSelectionVisible(columns, totalRowH, availableH);
        return true;
    }
    if (IsKeyPressed(key::RIGHT) && selectedChapter_ < chapterCount_) {
        selectedChapter_++;
        KeepSelectionVisible(columns, totalRowH, availableH);
        return true;
    }
    if (IsKeyPressed(key::UP)) {
        int target = selectedChapter_ - columns;
        if (target >= 1) {
            selectedChapter_ = target;
            KeepSelectionVisible(columns, totalRowH, availableH);
        }
        return true;
    }
    if (IsKeyPressed(key::DOWN)) {
        int target = selectedChapter_ + columns;
        if (target <= chapterCount_) {
            selectedChapter_ = target;
            KeepSelectionVisible(columns, totalRowH, availableH);
        }
        return true;
    }

    if (IsKeyPressed(key::ENTER)) {
        std::string ref = bookCode_ + "." + std::to_string(selectedChapter_);
        auto* bus = &eventBus_;
        navStack_.PopAll();
        bus->Emit(theword::event::NavigateEvent{ref});
        return true;
    }

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        tapDetector_.OnPress(GetMousePosition());

    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        Vector2 mousePos;
        auto tr = tapDetector_.OnRelease(GetMousePosition(), mousePos);
        if (tr == TapDetector::Result::Drag) { return false; }
        if (tr == TapDetector::Result::Tap) {

        float backW = uiScale_.dp(56);

        // Back button
        if (mousePos.y < headerH && mousePos.x < backW) {
            navStack_.Pop();
            return true;
        }

        // Grid click
        float totalRowWidth = columns * cellW + (columns - 1) * gap;
        float gridStartX = (screenW - totalRowWidth) / 2.0f;

        if (mousePos.y >= gridY && mousePos.y < gridY + availableH
            && mousePos.x >= gridStartX && mousePos.x < gridStartX + totalRowWidth) {
            int col = static_cast<int>((mousePos.x - gridStartX) / (cellW + gap));
            int row = static_cast<int>((mousePos.y - gridY + gridScrollY_) / totalRowH);
            int chapter = row * columns + col + 1;

            if (chapter >= 1 && chapter <= chapterCount_) {
                std::string ref = bookCode_ + "." + std::to_string(chapter);
                auto* bus = &eventBus_;
                navStack_.PopAll();
                bus->Emit(theword::event::NavigateEvent{ref});
                return true;
            }
        }
    }
    }

    return false;
}

void ChapterGridScreen::KeepSelectionVisible(int columns, float rowH, float visibleH) {
    int row = (selectedChapter_ - 1) / columns;
    float rowTop = row * rowH;
    if (rowTop < gridScrollY_) {
        gridScrollY_ = rowTop;
    } else if (rowTop + rowH > gridScrollY_ + visibleH) {
        gridScrollY_ = rowTop + rowH - visibleH;
    }
}

} // namespace theword::ui
