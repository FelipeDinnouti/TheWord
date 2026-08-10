#include "ChapterGridScreen.h"
#include "NavigationStack.h"
#include "components.h"
#include "core/Theme.h"
#include "core/Config.h"
#include "event/EventBus.h"
#include "event/Events.h"

namespace theword::ui {

using namespace theword::core;

ChapterGridScreen::ChapterGridScreen(NavigationStack& navStack,
                                     theword::event::EventBus& eventBus,
                                     const std::string& bookCode,
                                     const std::string& bookName,
                                     int chapterCount,
                                     const theword::core::UIScale& uiScale,
                                     int currentChapter)
    : navStack_(navStack), eventBus_(eventBus),
      bookCode_(bookCode), bookName_(bookName),
      chapterCount_(chapterCount), uiScale_(uiScale),
      tapDetector_(uiScale_.dp(10)) {
    if (currentChapter >= 1 && currentChapter <= chapterCount) {
        selectedChapter_ = currentChapter;
    }

    eventBus_.On<theword::event::ScrollEvent>(
        [this, alive = aliveGuard_](const theword::event::ScrollEvent& e) {
        if (!*alive) return;
        float screenH = uiScale_.screenH;
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

void ChapterGridScreen::Draw(theword::renderer::DrawContext& ctx) {
    const auto& palette = ctx.themeManager.Current();
    float screenW = ctx.uiScale.screenW;
    float screenH = ctx.uiScale.screenH;
    const Font& font = ctx.fonts.Get(theword::text::FontKind::Heading);
    float fontSize = ctx.fonts.HeadingSize();

    float headerH = uiScale_.dp(48);
    float padding = uiScale_.dp(12);
    float cellW = uiScale_.dp(56);
    float cellH = uiScale_.dp(48);
    float gap = uiScale_.dp(8);
    int columns = 5;

    DrawHeaderBar(ctx, font, fontSize, bookName_.c_str(), true);

    float gridY = headerH + padding;
    float totalRowWidth = columns * cellW + (columns - 1) * gap;
    float gridStartX = (screenW - totalRowWidth) / 2.0f;

    int totalRows = (chapterCount_ + columns - 1) / columns;
    float availableH = screenH - gridY - padding;
    float totalGridH = totalRows * (cellH + gap);
    float maxScroll = std::max(0.0f, totalGridH - availableH);
    gridScrollY_ = std::clamp(gridScrollY_, 0.0f, maxScroll);

    float labelSize = fontSize * 0.6f;

    // Clip grid content below the header bar
    ctx.PushClipRect(0, headerH, screenW, screenH - headerH);

    float scrollOffset = gridScrollY_;

    for (int ch = 1; ch <= chapterCount_; ++ch) {
        int col = (ch - 1) % columns;
        int row = (ch - 1) / columns;

        float cx = gridStartX + col * (cellW + gap);
        float cy = gridY + row * (cellH + gap) - scrollOffset;

        // Skip cells outside the visible area
        if (cy + cellH < 0 || cy > screenH) continue;

        std::string num = std::to_string(ch);
        Vector2 numSize = MeasureTextEx(font, num.c_str(), labelSize, 1);
        float numX = cx + (cellW - numSize.x) / 2.0f;
        float numY = cy + (cellH - numSize.y) / 2.0f;

        if (ch == selectedChapter_) {
            Color bg = palette.accentTeal;
            bg.a = 30;
            DrawRectangleRounded({cx, cy, cellW, cellH}, 0.15f, 8, bg);
            DrawRectangleRoundedLines({cx, cy, cellW, cellH}, 0.15f, 8, 1.5f, palette.accentTeal);
            DrawTextEx(font, num.c_str(), {numX, numY}, labelSize, 1, palette.accentTeal);
        } else {
            DrawTextEx(font, num.c_str(), {numX, numY}, labelSize, 1, palette.uiText);
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

    ctx.PopClipRect();

    bool overGrid = ctx.input.mouseY >= gridY && ctx.input.mouseY < gridY + availableH
                    && ctx.input.mouseX >= gridStartX && ctx.input.mouseX < gridStartX + totalRowWidth;
    ctx.SetCursor(overGrid ? MOUSE_CURSOR_POINTING_HAND : MOUSE_CURSOR_DEFAULT);
}

bool ChapterGridScreen::HandleInput(const theword::renderer::DrawContext& ctx, float /*deltaTime*/) {
    if (ctx.input.KeyPressed(key::ESCAPE)) {
        navStack_.Pop();
        return true;
    }

    float screenW = ctx.uiScale.screenW;
    float screenH = ctx.uiScale.screenH;
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
    float wheel = ctx.input.wheel;
    if (wheel != 0.0f) {
        gridScrollY_ = std::clamp(gridScrollY_ - wheel * totalRowH * 2.0f, 0.0f, maxScroll);
        return true;
    }

    if (ctx.input.KeyPressed(key::LEFT) && selectedChapter_ > 1) {
        selectedChapter_--;
        KeepSelectionVisible(columns, totalRowH, availableH);
        return true;
    }
    if (ctx.input.KeyPressed(key::RIGHT) && selectedChapter_ < chapterCount_) {
        selectedChapter_++;
        KeepSelectionVisible(columns, totalRowH, availableH);
        return true;
    }
    if (ctx.input.KeyPressed(key::UP)) {
        int target = selectedChapter_ - columns;
        if (target >= 1) {
            selectedChapter_ = target;
            KeepSelectionVisible(columns, totalRowH, availableH);
        }
        return true;
    }
    if (ctx.input.KeyPressed(key::DOWN)) {
        int target = selectedChapter_ + columns;
        if (target <= chapterCount_) {
            selectedChapter_ = target;
            KeepSelectionVisible(columns, totalRowH, availableH);
        }
        return true;
    }

    if (ctx.input.KeyPressed(key::ENTER)) {
        std::string ref = bookCode_ + "." + std::to_string(selectedChapter_);
        auto* bus = &eventBus_;
        navStack_.PopAll();
        bus->Emit(theword::event::NavigateEvent{ref});
        return true;
    }

    if (ctx.input.leftPressed)
        tapDetector_.OnPress(ctx.input.mouseX, ctx.input.mouseY);

    if (ctx.input.leftReleased) {
        float tapX, tapY;
        auto tr = tapDetector_.OnRelease(ctx.input.mouseX, ctx.input.mouseY, tapX, tapY);
        if (tr == TapDetector::Result::Drag) { return false; }
        if (tr == TapDetector::Result::Tap) {

        float backW = uiScale_.dp(56);

        // Back button
        if (tapY < headerH && tapX < backW) {
            navStack_.Pop();
            return true;
        }

        // Grid click
        float totalRowWidth = columns * cellW + (columns - 1) * gap;
        float gridStartX = (screenW - totalRowWidth) / 2.0f;

        if (tapY >= gridY && tapY < gridY + availableH
            && tapX >= gridStartX && tapX < gridStartX + totalRowWidth) {
            int col = static_cast<int>((tapX - gridStartX) / (cellW + gap));
            int row = static_cast<int>((tapY - gridY + gridScrollY_) / totalRowH);
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
