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

namespace {

std::vector<std::string> WrapText(const std::string& text, float maxWidth,
                                   const Font& font, float fontSize, int maxLines = 0) {
    std::vector<std::string> lines;
    std::string currentLine;
    float lineWidth = 0.0f;
    float spaceWidth = MeasureTextEx(font, " ", fontSize, 1).x;

    size_t start = 0;
    while (start < text.size()) {
        size_t space = text.find(' ', start);
        std::string word = (space == std::string::npos) ? text.substr(start) : text.substr(start, space - start);
        float wordWidth = MeasureTextEx(font, word.c_str(), fontSize, 1).x;

        float addedWidth = wordWidth;
        if (!currentLine.empty()) addedWidth += spaceWidth;

        if (lineWidth + addedWidth > maxWidth && !currentLine.empty()) {
            lines.push_back(currentLine);
            if (maxLines > 0 && static_cast<int>(lines.size()) >= maxLines) {
                lines.back() += "...";
                return lines;
            }
            currentLine = word;
            lineWidth = wordWidth;
        } else {
            if (!currentLine.empty()) currentLine += " ";
            currentLine += word;
            lineWidth += addedWidth;
        }

        start = (space == std::string::npos) ? text.size() : space + 1;
    }
    if (!currentLine.empty()) lines.push_back(currentLine);
    if (lines.empty()) lines.push_back(text);
    return lines;
}

} // anonymous namespace

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
      highlighter_(highlighter), uiScale_(uiScale) {
    eventBus_.On<theword::event::ScrollEvent>(
        [this, alive = aliveGuard_](const theword::event::ScrollEvent& e) {
        if (!*alive) return;
        float textWrapWidth = static_cast<float>(GetScreenWidth()) - uiScale_.dp(20) * 2;
        RebuildLayouts(textWrapWidth);
        if (layouts_.empty()) return;
        float totalH = 0;
        for (const auto& layout : layouts_) totalH += layout.height;
        float screenH = static_cast<float>(GetScreenHeight());
        float listY = uiScale_.dp(48) + uiScale_.dp(8) + uiScale_.dp(28) + uiScale_.dp(8);
        float listH = screenH - listY;
        float maxScroll = std::max(0.0f, totalH - listH);
        scrollY_ = std::clamp(scrollY_ + e.delta, 0.0f, maxScroll);
    });
}

HighlightBrowserScreen::~HighlightBrowserScreen() {
    *aliveGuard_ = false;
}

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

void HighlightBrowserScreen::RebuildLayouts(float textWrapWidth) const {
    bool filterChanged = (cachedFilterColorId_ != activeColorId_ ||
                          cachedHighlightsCount_ != highlighter_.GetHighlights().size());

    if (!filterChanged && lastTextWrapWidth_ == textWrapWidth && !layouts_.empty()) {
        return;
    }

    GetFilteredItems();

    float controlSize = fontSize_ * 0.65f;
    float subSize = controlSize * 0.85f;

    layouts_.clear();
    layouts_.reserve(cachedItems_.size());

    for (const auto& item : cachedItems_) {
        auto lines = WrapText(item.subtitle, textWrapWidth, font_, subSize, 0);
        float h = uiScale_.dp(4) + controlSize + uiScale_.dp(6) +
                  static_cast<float>(lines.size()) * (subSize * 1.4f) + uiScale_.dp(8);
        layouts_.push_back({h, std::move(lines)});
    }

    lastTextWrapWidth_ = textWrapWidth;
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
        DrawColorSwatch({swatchX, swatchRowY, swatchSize, swatchSize}, c, types[i].id == activeColorId_);
    }

    if (activeColorId_ == 0) {
        float allX = swatchStartX + types.size() * (swatchSize + swatchGap) + uiScale_.dp(8);
        DrawTextEx(font_, "(tap to filter)", {allX, swatchRowY + uiScale_.dp(4)},
                   controlSize, 1, GRAY);
    }

    // Highlight list
    float listY = swatchRowY + swatchSize + uiScale_.dp(8);
    float listH = screenH - listY;
    float textWrapWidth = screenW - uiScale_.dp(20) * 2;

    RebuildLayouts(textWrapWidth);

    // Empty state
    if (layouts_.empty()) {
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
        SetMouseCursor(MOUSE_CURSOR_DEFAULT);
        return;
    }

    // Compute total height and clamp scroll
    float totalHeight = 0;
    for (const auto& layout : layouts_) {
        totalHeight += layout.height;
    }
    float maxScroll = std::max(0.0f, totalHeight - listH);
    scrollY_ = std::min(scrollY_, maxScroll);

    // Draw visible items with pixel-based positioning
    float subSize = controlSize * 0.85f;
    float relY = 0;

    for (size_t i = 0; i < cachedItems_.size(); ++i) {
        const auto& item = cachedItems_[i];
        const auto& layout = layouts_[i];
        float screenY = listY + relY - scrollY_;

        if (screenY + layout.height >= 0 && screenY < screenH) {
            DrawTextEx(font_, item.title.c_str(),
                       {uiScale_.dp(20), screenY + uiScale_.dp(4)},
                       controlSize, 1, theme::UI_TITLE);

            float subY = screenY + uiScale_.dp(4) + controlSize + uiScale_.dp(6);
            for (const auto& line : layout.subtitleLines) {
                DrawTextEx(font_, line.c_str(),
                           {uiScale_.dp(20), subY},
                           subSize, 1, theme::UI_TEXT);
                subY += subSize * 1.4f;
            }
        }

        relY += layout.height;
    }

    Vector2 mouse = GetMousePosition();
    bool overInteractive = mouse.y > listY;
    SetMouseCursor(overInteractive ? MOUSE_CURSOR_POINTING_HAND : MOUSE_CURSOR_DEFAULT);
}

bool HighlightBrowserScreen::HandleInput(float /*deltaTime*/) {
    if (IsKeyPressed(key::ESCAPE)) {
        navStack_.Pop();
        return true;
    }

    float screenW = static_cast<float>(GetScreenWidth());
    float screenH = static_cast<float>(GetScreenHeight());
    float headerH = uiScale_.dp(48);
    float swatchRowY = headerH + uiScale_.dp(8);
    float swatchSize = uiScale_.dp(28);
    float swatchGap = uiScale_.dp(8);
    float swatchStartX = uiScale_.dp(16);
    float listY = swatchRowY + swatchSize + uiScale_.dp(8);
    float listH = screenH - listY;
    float textWrapWidth = screenW - uiScale_.dp(20) * 2;

    const auto& types = highlighter_.GetTypes();

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        pressStartPos_ = GetMousePosition();
        hasPendingPress_ = true;
    }

    if (hasPendingPress_ && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        hasPendingPress_ = false;
        Vector2 mousePos = GetMousePosition();
        float dx = mousePos.x - pressStartPos_.x;
        float dy = mousePos.y - pressStartPos_.y;
        if (dx * dx + dy * dy > uiScale_.dp(10) * uiScale_.dp(10)) return true;

        // Back button via header bar tap
        if (mousePos.y < headerH && mousePos.x < uiScale_.dp(56)) {
            navStack_.Pop();
            return true;
        }

        // Color swatch filter
        for (size_t i = 0; i < types.size(); ++i) {
            float swatchX = swatchStartX + i * (swatchSize + swatchGap);
            Rectangle swatchRect = {swatchX, swatchRowY, swatchSize, swatchSize};
            if (CheckCollisionPointRec(mousePos, swatchRect)) {
                activeColorId_ = (types[i].id == activeColorId_) ? 0 : types[i].id;
                scrollY_ = 0.0f;
                return true;
            }
        }

        // List item tap (pixel-based)
        if (mousePos.y >= listY) {
            RebuildLayouts(textWrapWidth);
            float relY = 0;
            for (size_t i = 0; i < layouts_.size(); ++i) {
                float screenY = listY + relY - scrollY_;
                if (mousePos.y >= screenY && mousePos.y < screenY + layouts_[i].height) {
                    OnItemTapped(static_cast<int>(i));
                    return true;
                }
                relY += layouts_[i].height;
            }
        }
    }

    // Scroll wheel
    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) {
        RebuildLayouts(textWrapWidth);
        if (!layouts_.empty()) {
            float totalHeight = 0;
            for (const auto& layout : layouts_) totalHeight += layout.height;
            float maxScroll = std::max(0.0f, totalHeight - listH);
            scrollY_ = std::clamp(scrollY_ - wheel * uiScale_.dp(48), 0.0f, maxScroll);
        }
        return true;
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
