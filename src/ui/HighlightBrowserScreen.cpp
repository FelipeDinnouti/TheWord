#include "HighlightBrowserScreen.h"
#include "NavigationStack.h"
#include "components.h"
#include "highlight/Highlighter.h"
#include "core/Theme.h"
#include "core/Config.h"
#include "event/EventBus.h"
#include "event/Events.h"
#include <algorithm>
#include <cmath>

namespace theword::ui {

using namespace theword::core;
using namespace theword::highlight;

HighlightBrowserScreen::HighlightBrowserScreen(const Font& font, float fontSize,
                                               NavigationStack& navStack,
                                               theword::event::EventBus& eventBus,
                                               const theword::highlight::Highlighter& highlighter,
                                               const theword::core::UIScale& uiScale)
    : font_(font), fontSize_(fontSize),
      navStack_(navStack), eventBus_(eventBus),
      highlighter_(highlighter), uiScale_(uiScale) {}

const std::vector<HighlightBrowserScreen::DisplayItem>& HighlightBrowserScreen::GetFilteredItems() const {
    const auto& all = highlighter_.GetHighlights();
    if (cachedFilterColorId_ == activeColorId_ && cachedHighlightsCount_ == all.size()) {
        return cachedItems_;
    }

    cachedItems_.clear();
    cachedItems_.reserve(all.size());

    for (const auto& h : all) {
        if (activeColorId_ != 0 && h.typeId != activeColorId_) continue;

        std::string title;
        if (h.bookId.empty()) {
            title = "Unknown";
        } else {
            title = h.bookId + "." + std::to_string(h.chapterNum);
            if (h.verseStart > 0) {
                title += ":" + std::to_string(h.verseStart);
                if (h.verseEnd > h.verseStart) {
                    title += "-" + std::to_string(h.verseEnd);
                }
            }
        }

        std::string subtitle;
        if (h.verseText.empty()) {
            subtitle = "(highlighted text)";
        } else {
            subtitle = h.verseText;
        }

        cachedItems_.push_back({&h, title, subtitle});
    }

    cachedFilterColorId_ = activeColorId_;
    cachedHighlightsCount_ = all.size();
    return cachedItems_;
}

void HighlightBrowserScreen::Draw() {
    float screenW = static_cast<float>(GetScreenWidth());
    float screenH = static_cast<float>(GetScreenHeight());

    DrawHeaderBar(font_, fontSize_, "Highlights", true, static_cast<int>(screenW), uiScale_);

    float headerH = uiScale_.dp(48);
    float controlSize = fontSize_ * 0.65f;

    // Color swatch filter row
    float swatchRowY = headerH + uiScale_.dp(8);
    float swatchSize = uiScale_.dp(28);
    float swatchGap = uiScale_.dp(8);
    float swatchStartX = uiScale_.dp(16);

    const auto& types = highlighter_.GetTypes();
    for (size_t i = 0; i < types.size(); ++i) {
        float swatchX = swatchStartX + i * (swatchSize + swatchGap);
        Color c = {types[i].color.r, types[i].color.g, types[i].color.b, 255};
        DrawRectangle(static_cast<int>(swatchX), static_cast<int>(swatchRowY),
                      static_cast<int>(swatchSize), static_cast<int>(swatchSize), c);
        DrawRectangleLinesEx(
            {swatchX - 1, swatchRowY - 1, swatchSize + 2, swatchSize + 2}, 2,
            types[i].id == activeColorId_ ? theme::PANEL_BORDER : theme::BUTTON_BORDER);
    }

    // "All" swatch / deselection hint
    if (activeColorId_ == 0) {
        float allX = swatchStartX + types.size() * (swatchSize + swatchGap) + uiScale_.dp(8);
        DrawTextEx(font_, "(tap to filter)", {allX, swatchRowY + uiScale_.dp(4)},
                   controlSize, 1, GRAY);
    }

    // Highlight list
    float listY = swatchRowY + swatchSize + uiScale_.dp(8);
    float listH = screenH - listY;
    float itemH = std::max(uiScale_.dp(48), controlSize * 2 + uiScale_.dp(16));

    const auto& items = GetFilteredItems();

    // Empty state
    if (items.empty()) {
        const char* msg;
        if (activeColorId_ == 0) {
            msg = "No highlights yet.\nSelect text in the Reader to create one.";
        } else {
            msg = "No highlights of this color.";
        }
        float msgSize = controlSize;
        Vector2 dims = MeasureTextEx(font_, msg, msgSize, 1);
        float x = (screenW - dims.x) / 2.0f;
        float y = listY + (listH - dims.y) / 2.0f;
        DrawTextEx(font_, msg, {x, y}, msgSize, 1, GRAY);
        return;
    }

    int maxScroll = std::max(0, static_cast<int>(items.size()) - static_cast<int>(listH / itemH));
    int scrollIdx = std::min(static_cast<int>(scrollOffset_), maxScroll);

    for (int i = scrollIdx; i < static_cast<int>(items.size()); ++i) {
        int visIdx = i - scrollIdx;
        float itemY = listY + visIdx * itemH;
        if (itemY + itemH > screenH) break;

        const auto& item = items[i];

        DrawTextEx(font_, item.title.c_str(),
                   {uiScale_.dp(16), itemY + uiScale_.dp(4)},
                   controlSize, 1, theme::UI_TITLE);
        DrawTextEx(font_, item.subtitle.c_str(),
                   {uiScale_.dp(16), itemY + controlSize + uiScale_.dp(6)},
                   controlSize * 0.85f, 1, theme::UI_TEXT);
    }
}

bool HighlightBrowserScreen::HandleInput(float /*deltaTime*/) {
    float screenH = static_cast<float>(GetScreenHeight());
    float headerH = uiScale_.dp(48);
    float controlSize = fontSize_ * 0.65f;

    if (IsKeyPressed(key::ESCAPE)) {
        navStack_.Pop();
        return true;
    }

    // Back button via header bar tap
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 mousePos = GetMousePosition();
        float backW = uiScale_.dp(56);

        if (mousePos.y < headerH && mousePos.x < backW) {
            navStack_.Pop();
            return true;
        }
    }

    // Color swatch filter
    float swatchRowY = headerH + uiScale_.dp(8);
    float swatchSize = uiScale_.dp(28);
    float swatchGap = uiScale_.dp(8);
    float swatchStartX = uiScale_.dp(16);

    const auto& types = highlighter_.GetTypes();

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 mousePos = GetMousePosition();

        for (size_t i = 0; i < types.size(); ++i) {
            float swatchX = swatchStartX + i * (swatchSize + swatchGap);
            Rectangle swatchRect = {swatchX, swatchRowY, swatchSize, swatchSize};
            if (CheckCollisionPointRec(mousePos, swatchRect)) {
                activeColorId_ = (types[i].id == activeColorId_) ? 0 : types[i].id;
                scrollOffset_ = 0.0f;
                return true;
            }
        }
    }

    // Scroll wheel
    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) {
        float listY = swatchRowY + swatchSize + uiScale_.dp(8);
        float listH = screenH - listY;
        float itemH = std::max(uiScale_.dp(48), controlSize * 2 + uiScale_.dp(16));
        const auto& items = GetFilteredItems();
        if (!items.empty()) {
            int maxScroll = std::max(0, static_cast<int>(items.size()) - static_cast<int>(listH / itemH));
            scrollOffset_ = std::clamp(scrollOffset_ - wheel, 0.0f, static_cast<float>(maxScroll));
        }
        return true;
    }

    // List item click
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 mousePos = GetMousePosition();
        float listY = swatchRowY + swatchSize + uiScale_.dp(8);
        float itemH = std::max(uiScale_.dp(48), controlSize * 2 + uiScale_.dp(16));

        if (mousePos.y >= listY) {
            int visIdx = static_cast<int>((mousePos.y - listY) / itemH);
            int scrollIdx = static_cast<int>(scrollOffset_);
            int itemIdx = scrollIdx + visIdx;
            const auto& items = GetFilteredItems();
            if (itemIdx >= 0 && itemIdx < static_cast<int>(items.size())) {
                OnItemTapped(itemIdx);
                return true;
            }
        }
    }

    return false;
}

void HighlightBrowserScreen::OnItemTapped(int index) {
    const auto& items = GetFilteredItems();
    if (index < 0 || index >= static_cast<int>(items.size())) return;
    NavigateToHighlight(*items[index].hl);
}

void HighlightBrowserScreen::NavigateToHighlight(const Highlight& h) {
    std::string bookId = h.bookId;
    int chapterNum = h.chapterNum;
    int startWord = h.startWord;
    auto& eventBus = eventBus_;
    navStack_.PopAll();
    if (bookId.empty()) return;
    std::string ref = bookId + "." + std::to_string(chapterNum);
    eventBus.Emit(theword::event::NavigateToHighlightEvent{ref, startWord});
}

} // namespace theword::ui
